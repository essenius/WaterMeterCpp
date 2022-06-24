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
#include "CppUnitTest.h"
#include "../WaterMeterCpp/FlowMeter.h"
#include "../WaterMeterCpp/EventServer.h"
#include "../WaterMeterCpp/EventClient.h"
#include <iostream>

#include "TestEventClient.h"
#include "FlowMeterDriver.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
    TEST_CLASS(FlowMeterTest) {
    public:
        static EventServer eventServer;
        TEST_METHOD(flowMeterFirstValueIsOutlierTest) {
            FlowMeterDriver actual(&eventServer);
            eventServer.subscribe(&actual, Topic::SensorWasReset);
            TestEventClient client(&eventServer);
            eventServer.subscribe(&client, Topic::ResetSensor);
            Coordinate sample{{3500, 3500}};
            actual.addSample(sample);
            assertResult(&actual, L"First measurement", 0, false, 0, true, false, false);
            Assert::AreEqual(4949.7474f, actual.getAverageAbsoluteDistance(), L"Absolute distance OK after first");
            sample.x = 1000;
            sample.y = 1000;
            for (int i = 0; i < 9; i++) {
                actual.addSample(sample);
                assertResult(&actual, L"Ignore outlier", i, false, 0, true, false, true);
                Assert::AreEqual(4949.7474f, actual.getAverageAbsoluteDistance(), L"Absolute distance not changed with outlier");

                Assert::AreEqual(0, client.getCallCount(), L"ResetSensor not published");
            }
            sample.x = 1020;
            sample.y = 1020;
            actual.addSample(sample);
            assertResult(&actual, L"Ignore outlier", 9, false, 0, true, false, true);
            Assert::AreEqual(1, client.getCallCount(), L"ResetSensor published");
            Assert::AreEqual(4949.7474f, actual.getAverageAbsoluteDistance(), L"Absolute distancestill  not changed with outlier");

            eventServer.publish(Topic::SensorWasReset, LONG_TRUE);
            // we're starting from scratch using the next value
            sample.x = 1500;
            sample.y = 1500;
            actual.addSample(sample);
            assertResult(&actual, L"first after outlier reset", 0, false, 0, true, false, false);
            Assert::AreEqual(2121.3203f, actual.getAverageAbsoluteDistance(), L"Absolute distance reset");

        }

        Coordinate getSample(const float sampleNumber, const float samplesPerCycle = 32, const float angleOffsetSamples = 0) const {
            constexpr float RADIUS = 10.0L;
            constexpr int16_t X_OFFSET = -100;
            constexpr int16_t Y_OFFSET = 100;
            const float angle = (sampleNumber - angleOffsetSamples) * PI / samplesPerCycle * 2.0f;
            return Coordinate {
	            {
		            static_cast<int16_t>(X_OFFSET + round(sin(angle) * RADIUS)),
					static_cast<int16_t>(Y_OFFSET + round(cos(angle) * RADIUS))
	            }
            };
        }

        TEST_METHOD(flowMeterGoodFlowTest) {
            FlowMeterDriver flowMeter(&eventServer);
            flowMeter.begin(4, 390);
            assertFloatAreEqual(0.8604f, flowMeter.getZeroThreshold(), L"Zero threshold", 0);

            int totalPeaks = 0;
            bool inFlow = false;
            int flowSwitches = 0;
            int flowLength = 0;
            int longestFlow = 0;

            // we take 4 ellipses with radius 10, with 32 samples for a compiete cycle
            Coordinate sample {};
            constexpr int STARTUP_IDLE_SAMPLES = 4;
            constexpr int SAMPLES_PER_CYCLE = 32;
            constexpr int SHUTDOWN_IDLE_SAMPLES = 88;
            constexpr int ANGLE_OFFSET_SAMPLES = SAMPLES_PER_CYCLE / 4;
            constexpr int CYCLES = 4;

            // smoothX, smoothY, highpassX, highpassY, averageAbsolutedistance, smoothDistance, angle
            float expected[][7] = {
				{-105.1087f, 94.4343f, -4.2356f, -4.0902f, 141.7776f, 7.6763f, -2.3737f}, // after cycle 1
                {-105.1048f, 94.4299f, -5.7681f, -3.6192f, 140.4442f, 7.0490f, -2.5812f}, // after cycle 2
                {-105.1048f, 94.4299f, -5.9877f, -3.5511f, 140.1859f, 6.9741f, -2.6063f}, // after cycle 3
                {-105.1048f, 94.4299f, -6.0191f, -3.5414f, 140.1359f, 6.9652f, -2.6098f}, // after cycle 4
            };

            sample = getSample(0, SAMPLES_PER_CYCLE, ANGLE_OFFSET_SAMPLES);
            for (int i=0; i<STARTUP_IDLE_SAMPLES; i++) {
                eventServer.publish(Topic::Sample, sample);
                if (i == 0) {
                    assertFlowAnalysis(&flowMeter, L"Init", i, -110.0f, 100.0f, 0, 0, 148.6607f, 0, -PI);
                }
                if (i == STARTUP_IDLE_SAMPLES - 1) {
                    assertFlowAnalysis(&flowMeter, L"Init", i, -110.0f, 100.0f, 0, 0, 148.6607f, 0, 0.7840f);
                }
                totalPeaks += flowMeter.isPeak();
            }
            assertResult(&flowMeter, L"After startup", 0, false, 0, true, false, false);
            Assert::AreEqual(0, totalPeaks, L"No peaks yet");
            Assert::IsFalse(flowMeter.hasFlow(), L"No flow yet");
            for (int cycle = 0; cycle < CYCLES; cycle++) {
                for (int sampleCount = 0; sampleCount < SAMPLES_PER_CYCLE; sampleCount++) {
                    sample = getSample(static_cast<float>(sampleCount), SAMPLES_PER_CYCLE, ANGLE_OFFSET_SAMPLES);
                    eventServer.publish(Topic::Sample, sample);
                    totalPeaks += flowMeter.isPeak();

                    if (flowMeter.hasFlow() != inFlow) {
                        inFlow = !inFlow;
                        flowSwitches++;
                        flowLength = 0;
                    }
                    if (inFlow) {
                        flowLength++;
                        if (flowLength > longestFlow) {
                            longestFlow = flowLength;
                        }
                    }
                }
                auto current = expected[cycle];
                assertFlowAnalysis(&flowMeter, L"Cycle", cycle, current[0], current[1], current[2], current[3], current[4], current[5], current[6]);
            }
            for (int i = 0; i < SHUTDOWN_IDLE_SAMPLES; i++) {
                eventServer.publish(Topic::Sample, sample);
            }
            assertFlowAnalysis(&flowMeter, L"shutdown", SHUTDOWN_IDLE_SAMPLES,  -110.0f, 98.0f, -0.0595f, 0.00578f, 147.2439f, 0.1347f, 3.04466f);
            assertResult(&flowMeter, L"Final", SHUTDOWN_IDLE_SAMPLES, false, 0, true, false, false);
		    Assert::AreEqual(4, totalPeaks, L"Found 4 peaks");
			Assert::AreEqual(1, flowSwitches, L"Flow switched once");
			Assert::AreEqual(110, longestFlow, L"Longest flow is 110 samples");
        }

        TEST_METHOD(flowMeterOutlierTest) {
            // redirect std::cout so you can see it
            std::streambuf* backup = std::cout.rdbuf();
            const std::stringstream ss;
            std::cout.rdbuf(ss.rdbuf());

            FlowMeter flowMeter(&eventServer);
            flowMeter.begin(1, 390.0f);
            int totalOutliers = 0;
            Coordinate testData[] = {
                {-81, -165}, { -80,-165 }, { -82,-164 }, { -80,-164 },
                { -82,-164 }, { -79,-165 }, { -82,-165 }, { -81,-165 },
                { -82,-165 }, { -81,-165 }, { -82,-165 }, { -79,-165 },
                { -81,-164 }, { -78,-161 }, { -78,-159 }, { -83,-158 },
                { -68,-154 }, { -80,-156 }, { -67,-150 }, { -75,-151 },
                { -65,-146 }, { -68,-148 }, { -62,-143 }, { -64,-148 },
                { -59,-140 }, { -55,-145 }, { -53,-139 }, { -46,-147 },
                { -44,-139 }, { -34,-150 }, { -37,-145 }, { -26,-157 }, {27, 27},
                { -33,-153 }, { -22,-168 }, { -28,-164 }, { -15,-182 },
                { -30,-179 }, { -22,-195 }, { -38,-189 }, { -25,-209 },
                { -45,-200 }, { -33,-215 }, { -53,-203 }, { -36,-221 },
                { -61,-206 }, { -45,-222 }, { -67,-208 }, { -47,-224 }, {27, 27},
                { -77,-205 }, { -54,-223 }, { -80,-205 }, { -60,-220 },
                { -89,-200 }, { -60,-219 }, { -93,-197 }, { -64,-216 },
                { -93,-193 }, { -68,-210 }, { -100,-185 }, { -67,-206 },
                { -104,-184 }, { -69,-203 }, { -100,-178 } };
            long index = 0;
            for (Coordinate entry : testData) {
                eventServer.publish(Topic::Sample, entry);
                totalOutliers += flowMeter.isOutlier();
                auto smooth = flowMeter.getSmoothSample();
                std::cout << smooth.x << "," << smooth.y << (flowMeter.isExcluded() ? "E" : "I") << "\n";
            }
            Assert::AreEqual(2, totalOutliers, L"Found 3 outliers");

            // restore original buffer for cout
            std::cout.rdbuf(backup);
        }

        TEST_METHOD(flowMeterResetSensorTest) {
            FlowMeterDriver actual(&eventServer);
            actual.begin(4, 390.0f);
            Assert::IsTrue(actual.wasReset(), L"was reset 1");
            for (int i=0; i<5; i++) {
                actual.addSample(getSample(static_cast<float>(i)));
            }
            assertFloatAreEqual(147.8443f, actual.getAverageAbsoluteDistance(), L"Average absolute distance OK before");
            assertFloatAreEqual(-0.3364f, actual.getAngle(), L"Angle OK before");
            assertFloatAreEqual(0.5748f, actual.getSmoothDistance(), L"Smooth distance OK before");

            Assert::IsFalse(actual.wasReset(), L"Not reset");
            actual.update(Topic::SensorWasReset, LONG_TRUE);
            Assert::IsTrue(actual.wasReset(), L"was reset 2");
            actual.addSample(getSample(5));
            assertFloatAreEqual(140.3567f, actual.getAverageAbsoluteDistance(), L"Average absolute distance OK after");
            assertFloatAreEqual(-3.1416f, actual.getAngle(), L"Angle OK before");
            assertFloatAreEqual(0.0f, actual.getSmoothDistance(), L"Smooth distance OK before");
        }

        TEST_METHOD(flowMeterSecondValueIsOutlierTest) {
            FlowMeterDriver actual(&eventServer);
            actual.begin(5, 390);
            actual.addSample(Coordinate{{3000, 2000}});
            assertResult(&actual, L"First measurement", 0, false, 0, true, false, false);
            assertFloatAreEqual(3605.5513f, actual.getAverageAbsoluteDistance(), L"Absolute distance OK before");

            actual.addSample(Coordinate{ 1500, 500 });
            // an outlier as second value should get ignored.
            assertResult(&actual, L"Early first outlier", 1, false, 0, true, false, true);
            assertFloatAreEqual(3605.5513f, actual.getAverageAbsoluteDistance(), L"Absolute distance did not change with outlier");

            actual.addSample(Coordinate { 3025, 1980 });
            assertResult(&actual, L"Fist after outlier", 2, false, 0, true, false, false);
            assertFloatAreEqual(3606.04297f, actual.getAverageAbsoluteDistance(), L"Absolute distance OK after");
        }


    private:
        void assertFlowAnalysis(const FlowMeterDriver* actual, const wchar_t* description, const int index,
            const float smoothX, const float smoothY, const float highPassX, const float highPassY, const float distance, const float smoothDistance, const float angle) const {
            std::wstring message(description);
            message += std::wstring(L" #") + std::to_wstring(index) + std::wstring(L" @ ");
            assertFloatAreEqual(smoothX, actual->getSmoothSample().x, (message + L"Smooth X").c_str());
            assertFloatAreEqual(smoothY, actual->getSmoothSample().y, (message + L"Smooth Y").c_str());
            assertFloatAreEqual(highPassX, actual->getHighPassSample().x, (message + L"High pass X").c_str());
            assertFloatAreEqual(highPassY, actual->getHighPassSample().y, (message + L"High pass Y").c_str());
            assertFloatAreEqual(distance, actual->getAverageAbsoluteDistance(), (message + L"AbsoluteDistance").c_str());
            assertFloatAreEqual(smoothDistance, actual->getSmoothDistance(), (message + L"SmoothDistance").c_str());
            assertFloatAreEqual(angle, actual->getAngle(), (message + L"angle").c_str());
        }

        void assertResult(const FlowMeter* meter, const wchar_t* description, const int index,
            const bool flow, const int zone, const bool searching, const bool peak, const bool outlier = false) const {
            std::wstring message(description);
            message += std::wstring(L" #") + std::to_wstring(index) + std::wstring(L" @ ");
            Assert::AreEqual(flow, meter->hasFlow(), (message + L"Flow").c_str());
            Assert::AreEqual(zone, meter->getZone(), (message + L"Zone").c_str());
            Assert::AreEqual(searching, meter->isSearching(), (message + L"Searching").c_str());
            Assert::AreEqual(peak, meter->isPeak(), (message + L"Peak").c_str());
            Assert::AreEqual(outlier, meter->isOutlier(), (message + L"Outlier").c_str());
        }

        static void addMeasurementSeries(FlowMeter* flowMeter, const int count, int (*f)(int)) {
            for (int i = 0; i < count; i++) {
                Coordinate sample{};
                sample.x = static_cast<int16_t>(f(i));
                sample.y = sample.x;
                flowMeter->addSample(sample);
            }
        }

        static void assertFloatAreEqual(
            const float expected, const float actual, const wchar_t* description = L"", const long index = 0) {
            std::wstring message(description);
            const float difference = fabsf(expected - actual);
            message += std::wstring(L". expected: ") + std::to_wstring(expected) +
                std::wstring(L" actual: " + std::to_wstring(actual) +
                    std::wstring(L" difference: ") + std::to_wstring(difference) +
                    std::wstring(L" #") + std::to_wstring(index));
            Assert::IsTrue(difference < 0.002f, message.c_str());
        }

    };
    EventServer FlowMeterTest::eventServer;
}
