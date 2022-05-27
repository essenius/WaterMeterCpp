// Copyright 2021-2022 Rik Essenius
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

#include <CppUnitTest.h>
#include <regex>

#include "../WaterMeterCpp/QueueClient.h"
#include "../WaterMeterCpp/SafeCString.h"
#include "AssertHelper.h"
#include "TestEventClient.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
    TEST_CLASS(QueueClientTest) {
    public:
        static EventServer eventServer;
        static Log logger;
        static TestEventClient testEventClient;

        TEST_METHOD(queueClientTest1) {
            uxQueueReset();
            uxRingbufReset();
            eventServer.subscribe(&testEventClient, Topic::Exclude);
            constexpr uint16_t QUEUE_SIZE = 20;
            QueueClient qClient(&eventServer, &logger, QUEUE_SIZE, 23);
            qClient.begin(qClient.getQueueHandle());
            eventServer.subscribe(&qClient, Topic::Exclude);
            for (int i = 0; i < QUEUE_SIZE; i++) {
                eventServer.publish(Topic::Exclude, i * 11);
            }

        	clearPrintOutput();
            // should not get saved
            eventServer.publish(Topic::Exclude, 12345);
            Assert::IsTrue(
                std::regex_match(
                    getPrintOutput(),
                    std::regex(R"(\[\] \[E\] Instance [0-9a-fA-F]+ \(23\): error sending \d+/12345\n\n)")),
                L"Log sent");
            testEventClient.reset();
            for (int i = 0; i < QUEUE_SIZE; i++) {
                char numbuf[10];
                Assert::IsTrue(qClient.receive(), L"receive true");
                Assert::AreEqual(i + 1, testEventClient.getCallCount(), L"Call count");
                safeSprintf(numbuf, "%d", 11 * i);
                Assert::AreEqual(numbuf, testEventClient.getPayload(), L"Payload");
            }
            Assert::IsFalse(qClient.receive(), L"Receive false");
            Assert::AreEqual<int>(QUEUE_SIZE, testEventClient.getCallCount(), L"Call count last");

            struct TestData {
                const char* input;
                const char* output;
                bool isLong;
                const wchar_t* description;
                bool operator<(const TestData& rhs) const noexcept
                {
                    return this->input < rhs.input; 
                }
            };

            const std::set<TestData> testData = {
                { "Test message", "Test message", false, L"Test message"},
                { "", "0", true, L"empty string"},
                { "1.0", "1.0", false, L"1.0 double"},
                { "1.a", "1.a", false, L"non-numerical value"},
                { "0", "0", true, L"zero"},
                { "2147483647", "2147483647",true, L"maxint32"},
                { "-2147483648", "-2147483648", true, L"minint32"}
            };

            const std::wstring message(L"long: ");
            for (auto iterator = testData.begin(); iterator != testData.end(); ++iterator) {
                eventServer.publish(Topic::Exclude, iterator->input);
                testEventClient.reset();
                Assert::IsTrue(qClient.receive(), L"received");
                Assert::AreEqual(iterator->output, testEventClient.getPayload(), L"Payload ok");
                Assert::AreEqual(iterator->isLong, testEventClient.wasLong(), (message + iterator->description).c_str());
            }

        }
    };

    EventServer QueueClientTest::eventServer;
    Log QueueClientTest::logger(&eventServer, nullptr);
    TestEventClient QueueClientTest::testEventClient(&eventServer);

}
