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

// ReSharper disable CppRedundantParentheses - intent clearer with the parentheses

#include "gtest/gtest.h"
#include "../WaterMeterCpp/ResultAggregator.h"
#include "../WaterMeterCpp/DataQueue.h"
#include "../WaterMeterCpp/Serializer.h"
#include "FlowMeterDriver.h"

#include "TestEventClient.h"

constexpr unsigned long MEASURE_INTERVAL_MICROS = 10UL * 1000UL;

namespace WaterMeterCppTest {

    class ResultAggregatorTest : public testing::Test {
    public:
        static EventServer eventServer;
        static DataQueuePayload payload;
        static TestEventClient rateListener;
        static PayloadBuilder payloadBuilder;
        static Serializer serializer;
        static DataQueue dataQueue;
        static Clock theClock;

        // ReSharper disable once CppInconsistentNaming
        static void SetUpTestCase() {
            eventServer.subscribe(&rateListener, Topic::Rate);
        }

        void SetUp() override {
            uxQueueReset();
            uxRingbufReset();
            rateListener.reset();
        }

    protected:
        void assertSummary(const int lastSample, const int sampleCount, const int peakCount, const int maxStreak, const ResultData* result) const {
            EXPECT_EQ(lastSample, static_cast<int>(result->lastSample.x)) << "Last sample OK";
            EXPECT_EQ(sampleCount, result->sampleCount) << "sampleCount OK";
            EXPECT_EQ(peakCount, result->pulseCount) << "pulseCount OK";
            EXPECT_EQ(maxStreak, result->maxStreak) << "maxStreak OK";
        }

        void assertExceptions(const int outliers, const int overruns, const ResultData* result) const {
            EXPECT_EQ(outliers, result->outlierCount) << "outlierCount OK";
            EXPECT_EQ(overruns, result->overrunCount) << "overrunCount OK";
        }

        void assertDuration(const int totalDuration, const int averageDuration, const int maxDuration, const ResultData* result) const {
            EXPECT_EQ(totalDuration, result->totalDuration) << "totalDuration OK";
            EXPECT_EQ(averageDuration, result->averageDuration) << "averageDuration OK";
            EXPECT_EQ(maxDuration, result->maxDuration) << "maxDuration OK";
        }
    };

    EventServer ResultAggregatorTest::eventServer;
    Clock ResultAggregatorTest::theClock(&eventServer);
    DataQueuePayload ResultAggregatorTest::payload;
    TestEventClient ResultAggregatorTest::rateListener(&eventServer);
    DataQueue ResultAggregatorTest::dataQueue(&eventServer, &payload);

    TEST_F(ResultAggregatorTest, resultAggregatorDisconnectTest) {
        ResultAggregator aggregator(&eventServer, &theClock, &dataQueue, &payload, MEASURE_INTERVAL_MICROS);
        aggregator.begin();
        eventServer.publish(Topic::IdleRate, 5);
        eventServer.publish(Topic::NonIdleRate, 5);
        constexpr FloatCoordinate LOW_PASS{500, 500};
        const FlowMeterDriver fmd(&eventServer, LOW_PASS);
        constexpr Coordinate SAMPLE{{500, 500}};
        for (int i = 0; i < 3; i++) {
            aggregator.addMeasurement(SAMPLE, &fmd);
            eventServer.publish(Topic::ProcessTime, 2500 + 10 * i);
            EXPECT_FALSE(aggregator.send()) << "First 3 measurements don't send";
        }
        setRingBufferBufferFull(dataQueue.handle(), true);

        for (int i = 0; i < 4; i++) {
            aggregator.addMeasurement(SAMPLE, &fmd);
            eventServer.publish(Topic::ProcessTime, 2500 - 10 * i);
            EXPECT_FALSE(aggregator.send()) << "Next 4 measurements still don't send (can't after 5th)";
        }

        setRingBufferBufferFull(dataQueue.handle(), false);

        for (int i = 0; i < 2; i++) {
            aggregator.addMeasurement(SAMPLE, &fmd);
            eventServer.publish(Topic::ProcessTime, 2510 - 10 * i);
            EXPECT_FALSE(aggregator.send()) << "Next 2 measurements still don't send (as waiting for next round";
        }
        aggregator.addMeasurement(SAMPLE, &fmd);
        eventServer.publish(Topic::ProcessTime, 2500);
        EXPECT_TRUE(aggregator.shouldSend()) << "next round complete, so must send";

        const auto result = &payload.buffer.result;

        // 10 samples (multiple of 5)
        assertSummary(500, 10, 0, 10, result);
        assertExceptions(0, 0, result);
        assertDuration(24980, 2498, 2520, result);
        // TODO: analysis of filtered values
    }

    // ReSharper disable once CyclomaticComplexity -- caused by EXPECT macros
    TEST_F(ResultAggregatorTest, resultAggregatorFlowTest) {
        ResultAggregator aggregator(&eventServer, &theClock, &dataQueue, &payload, MEASURE_INTERVAL_MICROS);
        aggregator.begin();
        eventServer.publish(Topic::IdleRate, 10);
        eventServer.publish(Topic::NonIdleRate, 5);
        for (int i = 0; i < 10; i++) {
            const auto measurement = static_cast<int16_t>(2400 + (i % 2) * 50);
            const Coordinate sample = {{measurement, measurement}};
            const float floatMeasurement = measurement;
            const FloatCoordinate smooth{floatMeasurement, floatMeasurement};

            FlowMeterDriver fmd(&eventServer, smooth, i == 5);

            aggregator.addMeasurement(sample, &fmd);
            eventServer.publish(Topic::ProcessTime, 8000 + i);
            EXPECT_EQ(i == 9, aggregator.shouldSend()) << "Should send(" << i << ")";
        }
        const auto result = &payload.buffer.result;

        // After 10 points, we get a summary with 1 pulse.
        assertSummary(2450, 10, 1, 0, result);
        assertExceptions(0, 0, result);
        assertDuration(80045, 8005, 8009, result);
        // TODO: analysis of filtered values
    }

    TEST_F(ResultAggregatorTest, resultAggregatorIdleOutlierTest) {
        ResultAggregator aggregator(&eventServer, &theClock, &dataQueue, &payload, MEASURE_INTERVAL_MICROS);
        aggregator.begin();
        eventServer.publish(Topic::IdleRate, "10");
        eventServer.publish(Topic::NonIdleRate, "5");
        EXPECT_EQ(2, rateListener.getCallCount()) << "two rate announcements";
        EXPECT_EQ(10L, aggregator.getFlushRate()) << "Flush rate at last set idle rate.";
        Coordinate sample{{0, 0}};

        for (int16_t i = 0; i < 15; i++) {
            sample.x = static_cast<int16_t>(2400 + (i > 7 ? 200 : 0));
            sample.y = sample.x;
            FloatCoordinate lowPass{};
            lowPass.set(sample);
            FlowMeterDriver fmd(&eventServer, lowPass, false, i > 7);
            aggregator.addMeasurement(sample, &fmd);
            eventServer.publish(Topic::ProcessTime, 7993 + i);
            EXPECT_EQ(i == 9 || i == 14, aggregator.shouldSend()) << "Must send - " << i;

            // we haven't sent yet, so #9 and #14 are still on the old flush rates (i.e. 5)
            EXPECT_EQ(i < 8 ? 10L : 5L, aggregator.getFlushRate()) << "FlushRate OK based on outliers - " << i;

            const auto result = &payload.buffer.result;
            if (i == 9) {
                // At 10 we get a summary with 2 outliers and 2 excludes
                assertSummary(2600, 10, 0, 8, result);
                assertExceptions(2, 0, result);
                assertDuration(79975, 7998, 8002, result);
                // TODO: analysis of filtered values
            }
            else if (i == 13) {
                EXPECT_TRUE(true);
            }
            else if (i == 14) {
                // At 15 we get the next batch with 5 outliers and excludes
                assertSummary(2600, 5, 0, 5, result);
                assertExceptions(5, 0, result);
                assertDuration(40025, 8005, 8007, result);
                // TODO: filtered value analysis
            }
            EXPECT_EQ(i == 9 || i == 14, aggregator.send()) << "Sending at 9 and 14";
        }
    }

    TEST_F(ResultAggregatorTest, resultAggregatorIdleTest) {
        ResultAggregator aggregator(&eventServer, &theClock, &dataQueue, &payload, MEASURE_INTERVAL_MICROS);
        aggregator.begin();

        EXPECT_EQ(1, rateListener.getCallCount()) << "rate set";
        EXPECT_EQ(6000L, aggregator.getFlushRate()) << "Default flush rate OK.";
        EXPECT_STREQ("6000", rateListener.getPayload()) << "rate was published";

        eventServer.publish(Topic::IdleRate, 10);
        EXPECT_EQ(2, rateListener.getCallCount()) << "rate set again";
        EXPECT_STREQ("10", rateListener.getPayload()) << "rate was taken from idlerate";
        EXPECT_EQ(10L, aggregator.getFlushRate()) << "Default flush rate OK.";

        eventServer.publish(Topic::NonIdleRate, 5);
        EXPECT_EQ(2, rateListener.getCallCount()) << "no new rate from non-idle";
        EXPECT_EQ(10L, aggregator.getFlushRate()) << "Flush rate not changed from non-idle.";

        for (int16_t i = 0; i < 10; i++) {
            const Coordinate sampleValue = {{static_cast<int16_t>(2400 + i), static_cast<int16_t>(1200 + i)}};
            FloatCoordinate lowPass{};
            lowPass.set(sampleValue);
            FlowMeterDriver fmd(&eventServer, lowPass);
            aggregator.addMeasurement(sampleValue, &fmd);
            eventServer.publish(Topic::ProcessTime, 1000 + i * 2);
            if (i == 0) {
                EXPECT_EQ(10L, aggregator.getFlushRate()) << "Flush rate stays idle when there is nothing special.";
            }
            EXPECT_EQ(i == 9, aggregator.shouldSend()) << "Should send - " << i;
            if (i == 9) {
                const auto result = &payload.buffer.result;

                assertSummary(2409, 10, 0, 0, result);
                assertExceptions(0, 0, result);
                assertDuration(10090, 1009, 1018, result);
                // TODO: filtered value analysis 
                aggregator.flush();
            }
        }
    }

    TEST_F(ResultAggregatorTest, resultAggregatorOverrunTest) {
        ResultAggregator aggregator(&eventServer, &theClock, &dataQueue, &payload, MEASURE_INTERVAL_MICROS);
        aggregator.begin();
        eventServer.publish(Topic::IdleRate, 1);
        eventServer.publish(Topic::NonIdleRate, 1);
        constexpr FloatCoordinate LOW_PASS{2400, 2400};
        const FlowMeterDriver fmd(&eventServer, LOW_PASS);
        aggregator.addMeasurement(Coordinate{{2398, 0}}, &fmd);
        eventServer.publish(Topic::ProcessTime, 10125L);
        EXPECT_TRUE(aggregator.shouldSend()) << "Needs flush";
        const auto result = &payload.buffer.result;

        // 1 point written, flush rate 1, measures contains sensor value, overrun triggered.
        assertSummary(2398, 1, 0, 0, result);
        assertExceptions(0, 1, result);
        assertDuration(10125, 10125, 10125, result);
        // TODO: filtered value analysis
    }
}
