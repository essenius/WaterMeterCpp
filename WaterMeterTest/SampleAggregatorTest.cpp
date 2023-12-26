// Copyright 2021-2023 Rik Essenius
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
#include "TestEventClient.h"
#include "../WaterMeter/DataQueue.h"
#include "../WaterMeter/SampleAggregator.h"

namespace WaterMeterCppTest {
    using namespace WaterMeter;

    TEST(SampleAggregatorTest, sampleAggregatorAddSampleTest) {
        EventServer eventServer;
        DataQueuePayload payload{};
        Clock theClock(&eventServer);
        DataQueue dataQueue(&eventServer, &payload);

        SampleAggregator aggregator(&eventServer, &theClock, &dataQueue, &payload);
        aggregator.begin();
        EXPECT_EQ(25L, aggregator.getFlushRate()) << "Default flush rate OK";
        eventServer.publish(Topic::BatchSizeDesired, "5");
        EXPECT_EQ(5L, aggregator.getFlushRate()) << "Flush rate changed";
        eventServer.publish(Topic::BatchSizeDesired, "DEFAULT");
        EXPECT_EQ(25L, aggregator.getFlushRate()) << "Flush rate changed back to default";
        eventServer.publish(Topic::BatchSizeDesired, 2);
        EXPECT_EQ(2L, aggregator.getFlushRate()) << "Flush rate changed";
        aggregator.flush();
        constexpr IntCoordinate Sample1{{1000, 1000}};
        aggregator.addSample(Sample1);
        EXPECT_FALSE(aggregator.shouldSend()) << "Should send";
        EXPECT_EQ(1U, static_cast<unsigned>(payload.buffer.samples.count)) << "One sample added";
        EXPECT_EQ(Sample1, payload.buffer.samples.value[0]) << "First sample value correct";

        constexpr IntCoordinate Sample2{{-1000, -1000}};

        aggregator.addSample(Sample2);
        EXPECT_TRUE(aggregator.shouldSend()) << "Needs flush after two measurements";

        // specialization for uint16_t does not work for some reason
        EXPECT_EQ(2U, static_cast<unsigned>(payload.buffer.samples.count)) << "Second sample added";
        EXPECT_EQ(Sample1, payload.buffer.samples.value[0]) << "First sample value still correct";
        EXPECT_EQ(Sample2, payload.buffer.samples.value[1]) << "Second sample value correct";

        aggregator.flush();
        EXPECT_EQ(0U, static_cast<unsigned>(payload.buffer.samples.count)) << "Buffer empty after flush";

    }

    // ReSharper disable once CyclomaticComplexity -- caused by EXPECT macros
    TEST(SampleAggregatorTest, sampleAggregatorZeroFlushRateTest) {
        EventServer eventServer;
        Clock theClock(&eventServer);
        DataQueuePayload payload{};
        DataQueue dataQueue(&eventServer, &payload);

        TestEventClient batchSizeListener(&eventServer);
        eventServer.subscribe(&batchSizeListener, Topic::BatchSize);
        SampleAggregator aggregator(&eventServer, &theClock, &dataQueue, &payload);
        aggregator.begin();

        EXPECT_FALSE(aggregator.send()) << "Should not send";

        EXPECT_EQ(1, batchSizeListener.getCallCount()) << "batch size set";
        EXPECT_STREQ("25", batchSizeListener.getPayload()) << "batch size is 50";
        EXPECT_EQ(25L, aggregator.getFlushRate()) << "Default flush rate OK";

        batchSizeListener.reset();
        eventServer.publish(Topic::BatchSizeDesired, 2L);
        EXPECT_EQ(1, batchSizeListener.getCallCount()) << "batch size changed (no measurements yet)";
        EXPECT_STREQ("2", batchSizeListener.getPayload()) << "batch size is 2";

        batchSizeListener.reset();
        IntCoordinate sample1{{1000, 1000}};
        aggregator.addSample(sample1);

        EXPECT_FALSE(aggregator.send()) << "No need to send after 1 measurement";

        // -1 should clip to 0;
        eventServer.publish(Topic::BatchSizeDesired, -1L);
        EXPECT_EQ(0, batchSizeListener.getCallCount()) << "batch size not changed";
        EXPECT_EQ(2L, aggregator.getFlushRate()) << "Flush rate not changed";
        IntCoordinate sample2{{3000, 3000}};
        aggregator.addSample(sample2);
        EXPECT_TRUE(aggregator.shouldSend()) << "Must send after two measurements";
        auto currentTimestamp = payload.timestamp;
        EXPECT_EQ(0ULL, currentTimestamp) << "Timestamp not set";
        EXPECT_EQ(sample1, payload.buffer.samples.value[0]) << "First value OK";
        EXPECT_EQ(sample2, payload.buffer.samples.value[1]) << "Second value OK";

        EXPECT_TRUE(aggregator.send()) << "Send successful";
        currentTimestamp = payload.timestamp;
        EXPECT_NE(0ULL, currentTimestamp) << "Timestamp set";

        EXPECT_EQ(0L, aggregator.getFlushRate()) << "Flush rate changed";
        aggregator.flush();
        IntCoordinate sample3{{4000, 4000}};
        aggregator.addSample(sample3);
        EXPECT_FALSE(aggregator.shouldSend()) << "No need to send";
        EXPECT_EQ(currentTimestamp, payload.timestamp) << "Timestamp not set";
        EXPECT_EQ(0U, static_cast<unsigned>(payload.buffer.samples.count)) << "Buffer empty";

        // check whether failure to write is handled OK
        eventServer.publish(Topic::BatchSizeDesired, 2L);
        EXPECT_EQ(2L, aggregator.getFlushRate()) << "Flush rate changed back to 2";
        IntCoordinate sample4{{-3000, -3000}};
        aggregator.addSample(sample4);
        setRingBufferBufferFull(dataQueue.handle(), true);
        aggregator.addSample(sample4);
        EXPECT_EQ(0U, static_cast<unsigned>(payload.buffer.samples.count)) << "Buffer flushed since we can't write";

        // reconnect
        setRingBufferBufferFull(dataQueue.handle(), false);
        IntCoordinate sample5{{-4000, -4000}};
        aggregator.addSample(sample5);
        EXPECT_EQ(1U, static_cast<unsigned>(payload.buffer.samples.count)) << "restarted filling buffer";

        EXPECT_FALSE(aggregator.shouldSend()) << "No flush needed after first";

        // Switch to max buffer size 
        batchSizeListener.reset();
        eventServer.publish(Topic::BatchSizeDesired, 10000L);
        IntCoordinate sample6{{-5000, -5000}};
        aggregator.addSample(sample6);
        EXPECT_TRUE(aggregator.send()) << "sends after reconnect";

        EXPECT_EQ(1, batchSizeListener.getCallCount()) << "batch size listener called once";
        EXPECT_STREQ("25", batchSizeListener.getPayload()) << "payload maximized at 25";
    }

    TEST(SampleAggregatorTest, sampleAggregator25SampleTest) {
        EventServer eventServer;
        Clock theClock(&eventServer);
        DataQueuePayload payload{};
        DataQueue dataQueue(&eventServer, &payload);

        TestEventClient batchSizeListener(&eventServer);
        eventServer.subscribe(&batchSizeListener, Topic::BatchSize);
        SampleAggregator aggregator(&eventServer, &theClock, &dataQueue, &payload);
        aggregator.begin();
        EXPECT_EQ(25L, aggregator.getFlushRate()) << "Default flush rate OK";

        IntCoordinate sample1{};
        for (int i = 0; i < 25; i++) {
            EXPECT_FALSE(aggregator.send()) << "no send before 25 samples";
            sample1.x = static_cast <int16_t>(-135 + i % 5);
            sample1.y = static_cast <int16_t>(-190 - i % 5);
            aggregator.addSample(sample1);
        }
        constexpr IntCoordinate Lastsample{ {-131, -194} };
        EXPECT_EQ(Lastsample, payload.buffer.samples.value[24]) << "Last sample value correct";
        EXPECT_TRUE(aggregator.send()) << "sends after 25 samples";
    }
}
