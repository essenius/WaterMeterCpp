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
            payload.buffer.result.smoothDistance = 23.2f;
            eventServer.publish(Topic::SensorData, reinterpret_cast<const char*>(&payload));
            Assert::AreEqual(1, testEventClient.getCallCount(), L"Test client called once result");
            Assert::AreEqual(

                R"({"timestamp":1970-01-01T00:00:00.000000,"last.x":0,"last.y":0,)"
                R"("summaryCount":{"samples":81,"peaks":3,"flows":27,"maxStreak":0},)"
                R"("exceptionCount":{"outliers":0,"excludes":0,"overruns":0,"resets":0},)"
                R"("duration":{"total":0,"average":0,"max":0},)"
                R"("analysis":{"lp.x":0,"lp.y":0,"hp.x":0,"hp.y":0,"angle":0,"distance":0,"smoothDistance":23.2}})",
                testEventClient.getPayload(),
                "Formatted result payload OK");

            testEventClient.reset();
            payload.topic = Topic::Samples;
            payload.buffer.samples.count = MAX_SAMPLES;
            for (uint16_t i = 0; i < payload.buffer.samples.count; i++) {
                const int16_t value = i + 475;
                payload.buffer.samples.value[i] = {{value, value}};
            }
            eventServer.publish(Topic::SensorData, reinterpret_cast<const char*>(&payload));
            Assert::AreEqual(1, testEventClient.getCallCount(), L"Test client called once max sample");
            Assert::AreEqual(
                R"({"timestamp":1970-01-01T00:00:00.000000,"measurements":)"
                R"([475,475,476,476,477,477,478,478,479,479,480,480,481,481,482,482,483,483,484,484,485,485,486,486,487,)"
                R"(487,488,488,489,489,490,490,491,491,492,492,493,493,494,494,495,495,496,496,497,497,498,498,499,499]})",
                testEventClient.getPayload(),
                "Formatted max sample payload OK");

            testEventClient.reset();
            payload.topic = Topic::Samples;
            payload.buffer.samples.count = 1;

            for (int16_t i = 0; i < 10; i++) {
                const int16_t value = i + 475;
                payload.buffer.samples.value[i] = { {value, value} };
            }
            eventServer.publish(Topic::SensorData, reinterpret_cast<const char*>(&payload));
            Assert::AreEqual(1, testEventClient.getCallCount(), L"Test client called once 1 sample");
            Assert::AreEqual(
                R"({"timestamp":1970-01-01T00:00:00.000000,"measurements":[475,475]})",
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
