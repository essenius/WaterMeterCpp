// Copyright 2022 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#include "pch.h"

#include <ESP.h>
#include <freertos/freeRTOS.h>

#include "CppUnitTest.h"
#include "TestEventClient.h"
#include "../WaterMeterCpp/DataQueue.h"
#include "../WaterMeterCpp/EventServer.h"
#include "../WaterMeterCpp/SafeCString.h"
// ReSharper disable once CppUnusedIncludeDirective -- false positive
#include "TopicHelper.h"
#include "../WaterMeterCpp/Serializer.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
    TEST_CLASS(DataQueueTest) {
public:
    TEST_METHOD(dataQueueTest1) {
        EventServer eventServer;
        Clock theClock(&eventServer);
        TestEventClient resultEventClient(&eventServer);
        TestEventClient sampleEventClient(&eventServer);
        TestEventClient errorEventClient(&eventServer);
        TestEventClient infoEventClient(&eventServer);
        TestEventClient freeSpaceEventClient(&eventServer);

        eventServer.subscribe(&resultEventClient, Topic::Result);
        eventServer.subscribe(&sampleEventClient, Topic::Samples);
        eventServer.subscribe(&errorEventClient, Topic::ConnectionError);
        eventServer.subscribe(&infoEventClient, Topic::Info);
        eventServer.subscribe(&freeSpaceEventClient, Topic::FreeQueueSize);

        // send a number of samples
        PayloadBuilder payloadBuilder;
        Serializer serializer(&eventServer, &payloadBuilder);
        DataQueuePayload payload{};
        DataQueue dataQueue(&eventServer, &payload, 1, 40960, 512, 4096);
        payload.topic = Topic::Samples;

        for (uint16_t i = 0; i < MAX_SAMPLES; i++) {
            payload.buffer.samples.value[i] = static_cast<int16_t>(i + 475);
        }

        for (uint16_t times = 0; times < 5; times++) {
            payload.buffer.samples.count = MAX_SAMPLES - times;
            auto size = DataQueue::requiredSize(payload.size());
            Assert::IsTrue(size <= 128, (L"Payload didn't max out at " + std::to_wstring(size)).c_str());
            Assert::IsTrue(dataQueue.send(&payload), (L"Send works for sample " + std::to_wstring(times)).c_str());
            Assert::AreEqual(1, freeSpaceEventClient.getCallCount(), (L"Free called right times at " + std::to_wstring(times)).c_str());
            Assert::AreEqual("16790016", freeSpaceEventClient.getPayload(), (L"right bytes free at "+ std::to_wstring(times)).c_str());
            delay(50);
        }

        // send a result
        payload.topic = Topic::Result;
        // clean out buffer to all 0
        for (short& i : payload.buffer.samples.value) {
            i = 0;
        }
        payload.buffer.samples.count = 0;

        payload.buffer.result.sampleCount = 81;
        payload.buffer.result.flowCount = 27;
        payload.buffer.result.peakCount = 3;
        payload.buffer.result.smoothAbsFastDerivative = 23.2f;
        dataQueue.send(&payload);

        // send an error message
        payload.topic = Topic::ConnectionError;
        safeStrcpy(payload.buffer.message, "Not sure what went wrong here...");
        dataQueue.send(&payload);

        // retrieve the samples
        for (int i = 0; i < 5; i++) {
            auto payloadReceive = dataQueue.receive();
            Assert::IsNotNull(payloadReceive, L"PayloadReceive not null 1");
            Assert::AreEqual(Topic::Samples, payloadReceive->topic, L"Topic is Samples");
        }
        Assert::AreEqual(2, freeSpaceEventClient.getCallCount(), L"Free called an extra time");
        Assert::AreEqual("16789376", freeSpaceEventClient.getPayload(), L"12160 + index #1)");

        // get the result
        auto payloadReceive2 = dataQueue.receive();
        Assert::IsNotNull(payloadReceive2, L"PayloadReceive not null 2");
        Assert::AreEqual(Topic::Result, payloadReceive2->topic, L"Topic 2 is Result");
        Assert::AreEqual<uint32_t>(81, payloadReceive2->buffer.result.sampleCount, L"SampleCount=81");
        Assert::AreEqual<uint32_t>(27, payloadReceive2->buffer.result.flowCount, L"FlowCount=27");
        Assert::AreEqual<uint32_t>(3, payloadReceive2->buffer.result.peakCount, L"PeakCount=3");
        Assert::AreEqual(23.2f, payloadReceive2->buffer.result.smoothAbsFastDerivative, L"SmoothAbsDerivativeSmooth=23.2");

        // get the error message
        auto payloadReceive3 = dataQueue.receive();
        Assert::IsNotNull(payloadReceive3, L"PayloadReceive not null 3");
        Assert::AreEqual(Topic::ConnectionError, payloadReceive3->topic, L"Topic 3 is SamplingError");
        Assert::AreEqual("Not sure what went wrong here...", payloadReceive3->buffer.message, L"error message ok");
    }

    };
}
