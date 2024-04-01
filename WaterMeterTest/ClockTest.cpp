// Copyright 2021-2024 Rik Essenius
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
#include "Clock.h"

namespace WaterMeterCppTest {
    using WaterMeter::Timestamp;
    using WaterMeter::Clock;
    using WaterMeter::EventServer;
    using WaterMeter::Topic;

    TEST(ClockTest, formatTimestampTest) {
        constexpr Timestamp Timestamp = 1;
        EventServer eventServer;
        // the mock only mocks the time setting and detection, but keeps the rest
        char destination[5] = R"(abcd)";
        EXPECT_FALSE(Clock::formatTimestamp(Timestamp, destination, sizeof destination)) << "Timestamp does not fit";
        EXPECT_STREQ("abcd", destination) << "Destination not changed";
    }

    TEST(ClockTest, test1) {
        EventServer eventServer;
        // the mock only mocks the time setting and detection, but keeps the rest
        Clock theClock(&eventServer);

        theClock.begin();

        // trigger the get function with Topic::Time
        const char* timestamp = eventServer.request(Topic::Time, "");
        EXPECT_EQ(std::size_t{26}, strlen(timestamp)) << "Length of timestamp ok";
        EXPECT_TRUE(std::regex_match(timestamp, std::regex(R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{6})"))) << "Time pattern matches";

        timestamp = theClock.get(Topic::BatchSize, nullptr);
        EXPECT_EQ(nullptr, timestamp) << "Unexpected topic returns default";

        eventServer.cannotProvide(&theClock);
        timestamp = eventServer.request(Topic::Time, "");
        EXPECT_STREQ("", timestamp) << "Time no longer available";

        eventServer.provides(&theClock, Topic::Time);
        timestamp = eventServer.request(Topic::Time, "");
        EXPECT_STRNE("", timestamp) << "Time filled again";

        eventServer.cannotProvide(&theClock, Topic::Time);
        timestamp = eventServer.request(Topic::Time, "");
        EXPECT_STREQ("", timestamp) << "Time no longer available";
    }
}
