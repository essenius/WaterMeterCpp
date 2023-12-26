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

#include "AggregatorDriver.h"
#include "gtest/gtest.h"

#include "../WaterMeter/Aggregator.h"
#include "../WaterMeter/Clock.h"
#include "../WaterMeter/DataQueue.h"

namespace WaterMeterCppTest {
    using WaterMeter::Clock;
    using WaterMeter::DataQueue;
    using WaterMeter::DataQueuePayload;

    TEST(AggregatorTest, aggregatorLimitTest) {
        EXPECT_EQ(5, AggregatorDriver::limit(-5, 5, 15));
        EXPECT_EQ(7, AggregatorDriver::limit(7, 5, 15));
        EXPECT_EQ(15, AggregatorDriver::limit(20, 5, 15));
    }

    TEST(AggregatorTest, aggregatorConvertToLongTest) {
        EXPECT_EQ(3, AggregatorDriver::convertToLong("DEFAULT", 3));
        EXPECT_EQ(7, AggregatorDriver::convertToLong("7", 3));
        EXPECT_EQ(-23, AggregatorDriver::convertToLong("-23", 3));
        EXPECT_EQ(0, AggregatorDriver::convertToLong("q", 3));
    }
    // ReSharper disable once CyclomaticComplexity
    TEST(AggregatorTest, aggregatorFlushTest) {
        EventServer eventServer;
        Clock theClock(&eventServer);
        theClock.begin();
        DataQueuePayload payload{};
        DataQueue dataQueue(&eventServer, &payload);
        EXPECT_EQ(0ULL, payload.timestamp);
        Aggregator aggregator(&eventServer, &theClock, &dataQueue, &payload);
        aggregator.begin(0);
        EXPECT_FALSE(aggregator.newMessage()) << "Can't create new message when flush rate is 0";
        aggregator.setDesiredFlushRate(2);
        EXPECT_EQ(2L, aggregator.getFlushRate()) << "Flush rate OK";
        EXPECT_FALSE(aggregator.shouldSend()) << "No need for flush before first message";
        aggregator.newMessage();
        EXPECT_EQ(0ULL, payload.timestamp) << "Timestamp not set";

        EXPECT_TRUE(aggregator.shouldSend(true)) << "Must send when forced even if not at target size.";
        EXPECT_FALSE(aggregator.shouldSend())  << "No need to send before being at target size";
        aggregator.newMessage();
        EXPECT_TRUE(aggregator.shouldSend()) << L"Must send after two messages";

        // force buffer to look like it's full
        setRingBufferBufferFull(dataQueue.handle(), true);
        EXPECT_TRUE(aggregator.shouldSend()) << "Should send";
        EXPECT_FALSE(aggregator.canSend()) << "Cannot send when buffer full";
        EXPECT_FALSE(aggregator.send()) << "Send fails";
        EXPECT_EQ(0ULL, payload.timestamp) << "Timestamp not set";

        aggregator.newMessage();
        EXPECT_FALSE(aggregator.shouldSend()) << "Still can't send when buffer is full";

        // force buffer to look like it got space again
        setRingBufferBufferFull(dataQueue.handle(), false);
        EXPECT_FALSE(aggregator.shouldSend()) << "Should not send due to not being at multiple of flush rate";
        EXPECT_TRUE(aggregator.canSend()) << "Can now send as there is space in the buffer";
        aggregator.newMessage();
        EXPECT_TRUE(aggregator.shouldSend()) << "Must send when at multiple of flush rate";
        aggregator.send();
        EXPECT_NE(0ULL, payload.timestamp) << "Timestamp set";
    }
}