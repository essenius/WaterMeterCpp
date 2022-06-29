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

#include "gtest/gtest.h"
#include <regex>
#include "../WaterMeterCpp/QueueClient.h"
#include "../WaterMeterCpp/SafeCString.h"
#include "TestEventClient.h"
#include "freertos/ringbuf.h"

namespace WaterMeterCppTest {
    
    class QueueClientTest : public testing::Test {
    public:
        static EventServer eventServer;
        static Log logger;
        static TestEventClient testEventClient;
    };

    EventServer QueueClientTest::eventServer;
    Log QueueClientTest::logger(&eventServer, nullptr);
    TestEventClient QueueClientTest::testEventClient(&eventServer);

    TEST_F(QueueClientTest, queueClientTest1) {
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
        const auto matcher = R"(\[\] \[E\] Instance [0-9a-fA-F]+ \(23\): error sending \d+/12345\n\n)";
        EXPECT_TRUE(std::regex_match(getPrintOutput(), std::regex(matcher))) << "Log sent";
        testEventClient.reset();
        for (int i = 0; i < QUEUE_SIZE; i++) {
            char numbuf[10];
            EXPECT_TRUE(qClient.receive()) << "receive true";
            EXPECT_EQ(i + 1, testEventClient.getCallCount()) << "Call count";
            safeSprintf(numbuf, "%d", 11 * i);
            EXPECT_STREQ(numbuf, testEventClient.getPayload()) << "Payload";
        }
        EXPECT_FALSE(qClient.receive()) << "Receive false";
        EXPECT_EQ(QUEUE_SIZE, testEventClient.getCallCount()) << "Call count last";

        struct TestData {
            const char* input;
            const char* output;
            bool isLong;
            const wchar_t* description;

            bool operator<(const TestData& rhs) const noexcept {
                return this->input < rhs.input;
            }
        };

        const std::set<TestData> testData = {
            {"Test message", "Test message", false, L"Test message"},
            {"", "0", true, L"empty string"},
            {"1.0", "1.0", false, L"1.0 double"},
            {"1.a", "1.a", false, L"non-numerical value"},
            {"0", "0", true, L"zero"},
            {"2147483647", "2147483647", true, L"maxint32"},
            {"-2147483648", "-2147483648", true, L"minint32"}
        };

        const std::wstring message(L"long: ");
        for (auto iterator = testData.begin(); iterator != testData.end(); ++iterator) {
            eventServer.publish(Topic::Exclude, iterator->input);
            testEventClient.reset();
            EXPECT_TRUE(qClient.receive()) << "received";
            EXPECT_STREQ(iterator->output, testEventClient.getPayload()) << "Payload ok";
            EXPECT_EQ(iterator->isLong, testEventClient.wasLong()) << message << iterator->description;
        }

    }
}
