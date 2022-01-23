// Copyright 2021 Rik Essenius
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
#include "CppUnitTest.h"
#include "../WaterMeterCpp/ResultAggregator.h"
#include "FlowMeterDriver.h"
#include <iostream>

#include "TestEventClient.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

constexpr unsigned long MEASURE_INTERVAL_MICROS = 10UL * 1000UL;

namespace WaterMeterCppTest {
    TEST_CLASS(ResultAggregatorTest) {
    public:
        static EventServer eventServer;
        static RingbufferPayload payload;
        static TestEventClient rateListener;
        static PayloadBuilder payloadBuilder;
        static Serializer serializer;
        static DataQueue dataQueue;

        TEST_CLASS_INITIALIZE(resultAggregatorTestClassInitialize) {
            eventServer.subscribe(&rateListener, Topic::Rate);
        }

        TEST_METHOD_INITIALIZE(resultAggregatorTestMethodInitialize) {
            rateListener.reset();
        }

        TEST_METHOD(resultAggregatorIdleTest) {
            ResultAggregator aggregator(&eventServer, &dataQueue, &payload, MEASURE_INTERVAL_MICROS);
            aggregator.begin();

            Assert::AreEqual(1, rateListener.getCallCount(), L"rate set");
            Assert::AreEqual(6000L, aggregator.getFlushRate(), L"Default flush rate OK.");
            Assert::AreEqual("6000", rateListener.getPayload(), L"rate was published");

            eventServer.publish(Topic::IdleRate, 10);
            Assert::AreEqual(2, rateListener.getCallCount(), L"rate set again");
            Assert::AreEqual("10", rateListener.getPayload(), L"rate was taken from idlerate");
            Assert::AreEqual(10L, aggregator.getFlushRate(), L"Default flush rate OK.");

            eventServer.publish(Topic::NonIdleRate, 5);
            Assert::AreEqual(2, rateListener.getCallCount(), L"no new rate from non-idle");
            Assert::AreEqual(10L, aggregator.getFlushRate(), L"Flush rate not changed from non-idle.");

            for (int16_t i = 0; i < 10; i++) {
                const auto sampleValue = static_cast<int16_t>(2400 + i);
                FlowMeterDriver fmd(&eventServer, sampleValue, 5, 7);
                aggregator.addMeasurement(sampleValue, &fmd);
                eventServer.publish(Topic::ProcessTime, 1000 + i * 2);
                if (i == 0) {
                    Assert::AreEqual(
                        10L,
                        aggregator.getFlushRate(),
                        L"Flush rate stays idle when there is nothing special.");
                }
                Assert::AreEqual(i == 9, aggregator.shouldSend());
                if (i == 9) {
                    const auto result = &payload.buffer.result;

                    AssertSummary(2409, 10, 0, 0, 1, result);
                    AssertExceptions(0, 0, 0, result);
                    AssertDuration(10090, 1009, 1018, result);
                    AssertAnalysis(2409.0f, 5.0f, 7.0f, 0.0f, result);
                    /*Assert::AreEqual(
                        R"({"timestamp":"","lastValue":2409,"summaryCount":{"samples":10,"peaks":0,"flows":0,"maxStreak":1},)"
                        R"("exceptionCount":{"outliers":0,"excludes":0,"overruns":0},)"
                        R"("duration":{"total":10090,"average":1009,"max":1018},)"
                        R"("analysis":{"smoothValue":2409,"derivative":5,"smoothDerivative":7,"smoothAbsDerivative":0}})",
                        writer.getMessage(),
                        L"10 idle points prepared after #10. "); */
                    aggregator.flush();
                }
            }
        }

        TEST_METHOD(resultAggregatorIdleOutlierTest) {
            ResultAggregator aggregator(&eventServer, &dataQueue, &payload, MEASURE_INTERVAL_MICROS);
            aggregator.begin();
            eventServer.publish(Topic::IdleRate, "10");
            eventServer.publish(Topic::NonIdleRate, "5");
            Assert::AreEqual(2, rateListener.getCallCount(), L"two rate announcements");
            Assert::AreEqual(10L, aggregator.getFlushRate(), L"Flush rate at last set idle rate.");

            for (int16_t i = 0; i < 15; i++) {
                const auto measurement = static_cast<int16_t>(2400 + (i > 7 ? 200 : 0));
                FlowMeterDriver fmd(&eventServer, measurement, -3, 1, false, false, i > 7, i > 7);
                aggregator.addMeasurement(measurement, &fmd);
                eventServer.publish(Topic::ProcessTime, 7993 + i);
                Assert::AreEqual(i == 9  || i == 14, aggregator.shouldSend(), (L"Must send - " + std::to_wstring(i)).c_str());

                // we haven't sent yet, so #9 and #14 are still on the old flush rates (i.e. 5)
                Assert::AreEqual(i < 8 ? 10L : 5L, aggregator.getFlushRate(), (L"FlushRate OK based on outliers - " + std::to_wstring(i)).c_str());

                const auto result = &payload.buffer.result;
                if (i == 9) {
                    // At 10 we get a summary with 2 outliers and 2 excludes
                    AssertSummary(2600, 10, 0, 0, 8, result);
                    AssertExceptions(2, 2, 0, result);
                    AssertDuration(79975, 7998, 8002, result);
                    AssertAnalysis(2600.0f, -3.0f, 1.0f, 0.0f, result);

                    /*Assert::AreEqual(
                        R"({"timestamp":"","lastValue":2600,"summaryCount":{"samples":10,"peaks":0,"flows":0,"maxStreak":8},)"
                        R"("exceptionCount":{"outliers":2,"excludes":2,"overruns":0},)"
                        R"("duration":{"total":79975,"average":7998,"max":8002},)"
                        R"("analysis":{"smoothValue":2400,"derivative":-3,"smoothDerivative":1,"smoothAbsDerivative":0}})",
                        writer.getMessage(),
                        "At 10 we get a summary with 2 outliers and 2 excludes"); */
                }
                else if (i==13) {
                    Assert::IsTrue(true);
                }
                else if (i == 14) {
                    // At 15 we get the next batch with 5 outliers and excludes
                    AssertSummary(2600, 5, 0, 0, 6, result);
                    AssertExceptions(5, 5, 0, result);
                    AssertDuration(40025, 8005, 8007, result);
                    AssertAnalysis(2600.0f, -3.0f, 1.0f, 0.0f, result);

                    /*Assert::AreEqual(
                        R"({"timestamp":"","lastValue":2600,"summaryCount":{"samples":5,"peaks":0,"flows":0,"maxStreak":6},)"
                        R"("exceptionCount":{"outliers":5,"excludes":5,"overruns":0},)"
                        R"("duration":{"total":40025,"average":8005,"max":8007},)"
                        R"("analysis":{"smoothValue":2400,"derivative":-3,"smoothDerivative":1,"smoothAbsDerivative":0}})",
                        writer.getMessage(),
                        "At 15 we get the next batch with 5 outliers and excludes"); */
                }
                Assert::AreEqual(i == 9 || i == 14, aggregator.send(), L"Sending at 9 and 14");
            }
        }

        TEST_METHOD(resultAggregatorOverrunTest) {
            ResultAggregator aggregator(&eventServer, &dataQueue, &payload, MEASURE_INTERVAL_MICROS);
            aggregator.begin();
            eventServer.publish(Topic::IdleRate, 1);
            eventServer.publish(Topic::NonIdleRate, 1);
            const FlowMeterDriver fmd(&eventServer, 2400, 0, 1);
            aggregator.addMeasurement(2398, &fmd);
            eventServer.publish(Topic::ProcessTime, 10125L);
            Assert::IsTrue(aggregator.shouldSend(), L"Needs flush");
            const auto result = &payload.buffer.result;

            // 1 point written, flush rate 1, measures contains sensor value, overrun triggered.
            AssertSummary(2398, 1, 0, 0, 1, result);
            AssertExceptions(0, 0, 1, result);
            AssertDuration(10125, 10125, 10125, result);
            AssertAnalysis(2400.0f, 0.0f, 1.0f, 0.0f, result);

            /*Assert::AreEqual(
                R"({"timestamp":"","lastValue":2398,"summaryCount":{"samples":1,"peaks":0,"flows":0,"maxStreak":1},)"
                R"("exceptionCount":{"outliers":0,"excludes":0,"overruns":1},)"
                R"("duration":{"total":10125,"average":10125,"max":10125},)"
                R"("analysis":{"smoothValue":2400,"derivative":0,"smoothDerivative":1,"smoothAbsDerivative":0}})",
                writer.getMessage(),
                L"1 point written, flush rate 1, measures contains sensor value, overrun triggered."); */
        }

        TEST_METHOD(resultAggregatorFlowTest) {
            ResultAggregator Aggregator(&eventServer, &dataQueue, &payload, MEASURE_INTERVAL_MICROS);
            Aggregator.begin();
            eventServer.publish(Topic::IdleRate, 10);
            eventServer.publish(Topic::NonIdleRate, 5);
            for (int i = 0; i < 10; i++) {
                const auto measurement = static_cast<int16_t>(2400 + (i % 2) * 50);
                FlowMeterDriver fmd(&eventServer, measurement, (i % 2) * 50, 1, i > 7, i == 5);

                Aggregator.addMeasurement(measurement, &fmd);
                eventServer.publish(Topic::ProcessTime, 8000 + i);
                Assert::AreEqual(i == 9, Aggregator.shouldSend());
            }
            const auto result = &payload.buffer.result;

            // After 10 points, we get a summary with 2 flows.
            AssertSummary(2450, 10, 1, 2, 1, result);
            AssertExceptions(0, 0, 0, result);
            AssertDuration(80045, 8005, 8009, result);
            AssertAnalysis(2450.0f, 50.0f, 1.0f, 0.0f, result);


            /*Assert::AreEqual(
                R"({"timestamp":"","lastValue":2450,"summaryCount":{"samples":10,"peaks":1,"flows":2,"maxStreak":1},)"
                R"("exceptionCount":{"outliers":0,"excludes":0,"overruns":0},)"
                R"("duration":{"total":80045,"average":8005,"max":8009},)"
                R"("analysis":{"smoothValue":2450,"derivative":50,"smoothDerivative":1,"smoothAbsDerivative":0}})",
                writer.getMessage(),
                L"After 10 points, we get a summary with 2 flows"); */
        }

        TEST_METHOD(resultAggregatorAllExcludedTest) {
            ResultAggregator aggregator(&eventServer, &dataQueue, &payload, MEASURE_INTERVAL_MICROS);
            aggregator.begin();
            eventServer.publish(Topic::IdleRate, 10);
            eventServer.publish(Topic::NonIdleRate, 5);

            const auto result = &(payload.buffer.result);

            for (int16_t i = 0; i < 15; i++) {
                FlowMeterDriver fmd(&eventServer, 2400 + (i == 2) * 10, 0, 0, false, false,
                                    i == 2, i == 2 || i == 3, i == 2);
                aggregator.addMeasurement(static_cast<int16_t>((i == 0) * 7600 + 2400 + (i > 7 ? 200 : 0)), &fmd);
                eventServer.publish(Topic::ProcessTime, i == 0 ? 2500 : 7500 + i * 5);
                Assert::AreEqual(i == 4 || i == 14, aggregator.shouldSend(), L"Must send at 5 and 15");
                Assert::AreEqual(i < 2 || i > 4 ? 10L : 5L, aggregator.getFlushRate(),
                                 L"Flush rate is 10 the first two data points, and after the 5th. In between it is 5");
                if (i == 4) {
                    // The right data prepared after 5 points 
                    AssertSummary(2400, 5, 0, 0, 4, result);
                    AssertExceptions(1, 4, 0, result);
                    AssertDuration(32550, 6510, 7520, result);
                    AssertAnalysis(2400.0f, 0.0f, 0.0f, 0.0f, result);
                    /*Assert::AreEqual(
                        R"({"timestamp":"","lastValue":2400,"summaryCount":{"samples":5,"peaks":0,"flows":0,"maxStreak":4},)"
                        R"("exceptionCount":{"outliers":1,"excludes":4,"overruns":0},)"
                        R"("duration":{"total":32550,"average":6510,"max":7520},)"
                        R"("analysis":{"smoothValue":2400,"derivative":0,"smoothDerivative":0,"smoothAbsDerivative":0}})",
                        writer.getMessage(),
                        L"The right data prepared after 5 points"); */
                    Assert::IsTrue(aggregator.send(), L"sending");

                }
            }
            // Without special events, the next flush is after 10 data points
            AssertSummary(2600, 10, 0, 0, 7, result);
            AssertExceptions(0, 0,0, result);
            AssertDuration(75475, 7548, 7570, result);
            AssertAnalysis(2400.0f, 0.0f, 0.0f, 0.0f, result);
            /*Assert::AreEqual(
                R"({"timestamp":"","lastValue":2600,"summaryCount":{"samples":10,"peaks":0,"flows":0,"maxStreak":7},)"
                R"("exceptionCount":{"outliers":0,"excludes":0,"overruns":0},)"
                R"("duration":{"total":75475,"average":7548,"max":7570},)"
                R"("analysis":{"smoothValue":2400,"derivative":0,"smoothDerivative":0,"smoothAbsDerivative":0}})",
                writer.getMessage(),
                L"Without special events, the next flush is after 10 data points"); */
        }

        TEST_METHOD(resultAggregatorDisconnectTest) {
            ResultAggregator aggregator(&eventServer, &dataQueue, &payload, MEASURE_INTERVAL_MICROS);
            aggregator.begin();
            eventServer.publish(Topic::IdleRate, 5);
            eventServer.publish(Topic::NonIdleRate, 5);
            const FlowMeterDriver fmd(&eventServer, 500);
            for (int i = 0; i < 3; i++) {
                aggregator.addMeasurement(500, &fmd);
                eventServer.publish(Topic::ProcessTime, 2500 + 10 * i);
                Assert::IsFalse(aggregator.send(), L"First 3 measurements don't send");
            }
            setRingBufferBufferFull(true);
            //eventServer.publish(Topic::Disconnected, LONG_TRUE);
            for (int i = 0; i < 4; i++) {
                aggregator.addMeasurement(500, &fmd);
                eventServer.publish(Topic::ProcessTime, 2500 - 10 * i);
                Assert::IsFalse(aggregator.send(), L"Next 4 measurements still don't send (can't after 5th)");
            }

            setRingBufferBufferFull(false);

            //eventServer.publish(Topic::Connected, 1);
            for (int i = 0; i < 2; i++) {
                aggregator.addMeasurement(500, &fmd);
                eventServer.publish(Topic::ProcessTime, 2510 - 10 * i);
                Assert::IsFalse(aggregator.send(),
                                L"Next 2 measurements still don't send (as waiting for next round");
            }
            aggregator.addMeasurement(500, &fmd);
            eventServer.publish(Topic::ProcessTime, 2500);
            Assert::IsTrue(aggregator.shouldSend(), L"next round complete, so must send");

            const auto result = &payload.buffer.result;

            // 10 samples (multiple of 5)
            AssertSummary(500, 10, 0, 0, 10, result);
            AssertExceptions(0, 0, 0, result);
            AssertDuration(24980, 2498, 2520, result);
            AssertAnalysis(500.0f, 0.0f, 0.0f, 0.0f, result);

            /*Assert::AreEqual(
                R"({"timestamp":"","lastValue":500,"summaryCount":{"samples":10,"peaks":0,"flows":0,"maxStreak":10},)"
                R"("exceptionCount":{"outliers":0,"excludes":0,"overruns":0},)"
                R"("duration":{"total":24980,"average":2498,"max":2520},)"
                R"("analysis":{"smoothValue":500,"derivative":0,"smoothDerivative":0,"smoothAbsDerivative":0}})",
                writer.getMessage(),
                "10 samples (multiple of 5)"); */
        }
        private:
            void AssertSummary(const int lastSample, const int sampleCount, const int peakCount, const int flowCount, const int maxStreak, const ResultData* result) const {
                Assert::AreEqual(lastSample, static_cast<int>(result->lastSample), L"Last sample OK");
                Assert::AreEqual<uint32_t>(sampleCount, result->sampleCount, L"sampleCount OK");
                Assert::AreEqual<uint32_t>(peakCount, result->peakCount, L"peakCount OK");
                Assert::AreEqual<uint32_t>(flowCount, result->flowCount, L"flowCount OK");
                Assert::AreEqual<uint32_t>(maxStreak, result->maxStreak, L"maxStreak OK");
            }

            void AssertExceptions(const int outliers, const int excludes, const int overruns, const ResultData* result) const {
                Assert::AreEqual<uint32_t>(outliers, result->outlierCount, L"outlierCount OK");
                Assert::AreEqual<uint32_t>(excludes, result->excludeCount, L"excludeCount OK");
                Assert::AreEqual<uint32_t>(overruns, result->overrunCount, L"overrunCount OK");
            }

            void AssertDuration(const int totalDuration, const int averageDuration, const int maxDuration, const ResultData* result) const {
                Assert::AreEqual<uint32_t>(totalDuration, result->totalDuration, L"totalDuration OK");
                Assert::AreEqual<uint32_t>(averageDuration, result->averageDuration, L"averageDuration OK");
                Assert::AreEqual<uint32_t>(maxDuration, result->maxDuration, L"maxDuration OK");
            }

            void AssertAnalysis(const float smooth, const float derivativeSmooth, const float smoothDerivativeSmooth, const float smoothAbsDerivativeSmooth, const ResultData* result) const {
                Assert::AreEqual(smooth, result->smooth, L"smooth OK");
                Assert::AreEqual(derivativeSmooth, result->derivativeSmooth, L"derivativeSmooth OK");
                Assert::AreEqual(smoothDerivativeSmooth, result->smoothDerivativeSmooth, L"smoothDerivativeSmooth OK");
                Assert::AreEqual(smoothAbsDerivativeSmooth, result->smoothAbsDerivativeSmooth, L"smoothAbsDerivativeSmooth OK");
            }

    };

    EventServer ResultAggregatorTest::eventServer;
    RingbufferPayload ResultAggregatorTest::payload;
    TestEventClient ResultAggregatorTest::rateListener(&eventServer);
    PayloadBuilder ResultAggregatorTest::payloadBuilder;
    Serializer ResultAggregatorTest::serializer(&payloadBuilder);
    DataQueue ResultAggregatorTest::dataQueue(&eventServer, &serializer);
}