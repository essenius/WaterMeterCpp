// Copyright 2022 Rik Essenius
// 
//   Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//   except in compliance with the License. You may obtain a copy of the License at
// 
//       http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software distributed under the License
//    is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and limitations under the License.

#include "pch.h"

#include <cstdlib>

#include "CppUnitTest.h"
#include "TestEventClient.h"
#include "../WaterMeterCpp/DataQueue.h"
#include "../WaterMeterCpp/ArduinoMock.h"
#include "../WaterMeterCpp/EventServer.h"
#include "../WaterMeterCpp/FreeRtosMock.h"
#include "../WaterMeterCpp/SafeCString.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
    TEST_CLASS(DataQueueTest){
public:
    TEST_METHOD(dataQueueTest1) {
        EventServer eventServer;
        Clock theClock(&eventServer);
        TestEventClient resultEventClient(&eventServer);
        TestEventClient sampleEventClient(&eventServer);
        TestEventClient errorEventClient(&eventServer);
        TestEventClient infoEventClient(&eventServer);

        eventServer.subscribe(&resultEventClient, Topic::Result);
        eventServer.subscribe(&sampleEventClient, Topic::Samples);
        eventServer.subscribe(&errorEventClient, Topic::SamplingError);
        eventServer.subscribe(&infoEventClient, Topic::Info);

        PayloadBuilder payloadBuilder;
        Serializer serializer(&payloadBuilder);

        DataQueue dataQueue(&eventServer, &theClock, &serializer);
        RingbufferPayload payload{};
        payload.topic = Topic::Samples;
        for (uint16_t times = 0; times < 5; times++) {
            payload.buffer.samples.count = MAX_SAMPLES - times;
            for (int16_t i = 0; i < payload.buffer.samples.count; i++) {
                payload.buffer.samples.value[i] = static_cast<int16_t>(i + 475);
            }
            auto size = dataQueue.requiredSize(dataQueue.payloadSize(&payload));
            Assert::IsTrue(size <= 128, (L"Payload didn't max out: " + std::to_wstring(size)).c_str());
            Assert::IsTrue(dataQueue.send(&payload), (L"Send works for sample " + std::to_wstring(times)).c_str());
            Serial.printf("<<P%d Stack size: %d>>", times, uxTaskGetStackHighWaterMark(nullptr));
            delay(50);
        }
        payload.topic = Topic::Result;
        // clean out buffer to all 0
        for (short& i : payload.buffer.samples.value) {
            i = 0;
        }
        payload.buffer.samples.count = 0;

        payload.buffer.result.sampleCount = 81;
        payload.buffer.result.flowCount = 27;
        payload.buffer.result.peakCount = 3;
        dataQueue.send(&payload);

        payload.topic = Topic::SamplingError;
        safeStrcpy(payload.buffer.message, "Not sure what went wrong here...");
        dataQueue.send(&payload);

        payload.topic = Topic::Info;
        safeStrcpy(payload.buffer.message, "About to close down...");
        dataQueue.send(&payload);

        while (dataQueue.receive()) {
            delay(100);
        }
        // skip timestamps
        Assert::AreEqual(" Error: Not sure what went wrong here...", errorEventClient.getPayload()+26);
        Assert::AreEqual(" Info: About to close down...", infoEventClient.getPayload()+26);
        Assert::AreEqual(5, sampleEventClient.getCallCount());
        Assert::AreEqual(R"([475,476,477,478,479,480,481,482,483,484,485,486,487,488,489,490,491,492,493,494,495,496,497,498,499,500,501,502,503,504,505,506,507,508,509,510,511,512,513,514,515,516,517,518,519,520]})", sampleEventClient.getPayload()+55);
        Assert::AreEqual(R"(,"lastValue":0,"summaryCount":{"samples":81,"peaks":3,"flows":27,"maxStreak":0},"exceptionCount":{"outliers":0,"excludes":0,"overruns":0},"duration":{"total":0,"average":0,"max":0},"analysis":{"smoothValue":0,"derivative":0,"smoothDerivative":0,"smoothAbsDerivative":0}})", resultEventClient.getPayload()+39);

    }
    };
}
