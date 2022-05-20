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

#include "CppUnitTest.h"
#include "TestEventClient.h"
#include "../WaterMeterCpp/Serializer.h"
#include "../WaterMeterCpp/SafeCString.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
    TEST_CLASS(SerializerTest) {
    public:
        TEST_METHOD(serializerTest1) {
            PayloadBuilder payloadBuilder;
            EventServer eventServer;
            Serializer serializer(&eventServer, &payloadBuilder);
            TestEventClient testEventClient(&eventServer);
            eventServer.subscribe(&testEventClient, Topic::ResultFormatted);
            eventServer.subscribe(&testEventClient, Topic::SamplesFormatted);
            eventServer.subscribe(&testEventClient, Topic::ErrorFormatted);
            eventServer.subscribe(&testEventClient, Topic::MessageFormatted);
            eventServer.subscribe(&serializer, Topic::SensorData);
            DataQueuePayload payload{};
            payload.topic = Topic::Result;
            payload.timestamp = 0;
            payload.buffer.result.sampleCount = 81;
            payload.buffer.result.peakCount = 3;
            payload.buffer.result.flowCount = 27;
            payload.buffer.result.smoothAbsFastDerivative = 23.2f;
            eventServer.publish(Topic::SensorData, reinterpret_cast<const char*>(&payload));
            Assert::AreEqual(1, testEventClient.getCallCount(), L"Test client called once result");
            Assert::AreEqual(
                R"({"timestamp":1970-01-01T00:00:00.000000,"lastValue":0,"summaryCount":{"samples":81,"peaks":3,"flows":27,"maxStreak":0},)"
                R"("exceptionCount":{"outliers":0,"excludes":0,"overruns":0,"resets":0},"duration":{"total":0,"average":0,"max":0},)"
                R"("analysis":{"LPF":0,"HPLPF":0,"LPHPF":0,"LPAHPLPF":23.2,"LFS":0,"HPC":0,"LPAHPC":0}})",
                testEventClient.getPayload(),
                "Formatted result payload OK");

            testEventClient.reset();
            payload.topic = Topic::Samples;
            payload.buffer.samples.count = MAX_SAMPLES;
            for (uint16_t i = 0; i < payload.buffer.samples.count; i++) {
                payload.buffer.samples.value[i] = static_cast<int16_t>(i + 475);
            }
            eventServer.publish(Topic::SensorData, reinterpret_cast<const char*>(&payload));
            Assert::AreEqual(1, testEventClient.getCallCount(), L"Test client called once max sample");
            Assert::AreEqual(
                R"({"timestamp":1970-01-01T00:00:00.000000,"measurements":)"
                R"([475,476,477,478,479,480,481,482,483,484,485,486,487,488,489,490,491,492,493,494,495,496,497,498,499,)"
                R"(500,501,502,503,504,505,506,507,508,509,510,511,512,513,514,515,516,517,518,519,520,521,522,523,524]})",
                testEventClient.getPayload(),
                "Formatted max sample payload OK");

            testEventClient.reset();
            payload.topic = Topic::Samples;
            payload.buffer.samples.count = 1;

            for (int16_t i = 0; i < 10; i++) {
                payload.buffer.samples.value[i] = static_cast<int16_t>(i + 475);
            }
            eventServer.publish(Topic::SensorData, reinterpret_cast<const char*>(&payload));
            Assert::AreEqual(1, testEventClient.getCallCount(), L"Test client called once 1 sample");
            Assert::AreEqual(
                R"({"timestamp":1970-01-01T00:00:00.000000,"measurements":[475]})",
                testEventClient.getPayload(),
                "Formatted 1 sample payload OK");

            testEventClient.reset();
            payload.topic = Topic::Samples;
            payload.buffer.samples.count = 0;
            eventServer.publish(Topic::SensorData, reinterpret_cast<const char*>(&payload));
            Assert::AreEqual(1, testEventClient.getCallCount(), L"Test client called once empty");
            Assert::AreEqual(
                R"({"timestamp":1970-01-01T00:00:00.000000,"measurements":[]})",
                testEventClient.getPayload(),
                "Formatted empty sample payload OK");

            testEventClient.reset();
            payload.topic = Topic::ConnectionError;
            safeStrcpy(payload.buffer.message, "Not sure what went wrong here...");
            eventServer.publish(Topic::SensorData, reinterpret_cast<const char*>(&payload));
            Assert::AreEqual(1, testEventClient.getCallCount(), L"Test client called once error");
            Assert::AreEqual(
                "Error: Not sure what went wrong here...",
                testEventClient.getPayload(),
                "Formatted Error payload OK");

            testEventClient.reset();
            payload.topic = Topic::Info;
            safeStrcpy(payload.buffer.message, "About to close down");
            eventServer.publish(Topic::SensorData, reinterpret_cast<const char*>(&payload));
            Assert::AreEqual(1, testEventClient.getCallCount(), L"Test client called once info");
            Assert::AreEqual("About to close down", testEventClient.getPayload(), "Formatted Info payload OK");
        }
    };
}
