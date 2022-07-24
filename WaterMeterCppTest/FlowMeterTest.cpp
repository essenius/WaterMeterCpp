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
#include "../WaterMeterCpp/FlowMeter.h"
#include "../WaterMeterCpp/EventServer.h"
#include "../WaterMeterCpp/EventClient.h"
#include <iostream>

#include "TestEventClient.h"
#include "FlowMeterDriver.h"

namespace WaterMeterCppTest {
    
    class FlowMeterTest : public testing::Test {
    public:
        static EventServer eventServer;
    protected:
        void expectResult(const FlowMeter* meter, const wchar_t* description, const int index,
                          const bool flow, const int zone, const bool searching, const bool peak,
                          const bool outlier = false) const {
            std::wstring message(description);
            message += std::wstring(L" #") + std::to_wstring(index) + std::wstring(L" @ ");
            EXPECT_EQ(flow, meter->hasFlow()) << message << "Flow";
            EXPECT_EQ(zone, meter->getZone()) << message << "Zone";
            EXPECT_EQ(searching, meter->isSearching()) << message << "Searching";
            EXPECT_EQ(peak, meter->isPeak()) << message << "Peak";
            EXPECT_EQ(outlier, meter->isOutlier()) << message << "Outlier";
        }

        Coordinate getSample(const float sampleNumber, const float samplesPerCycle = 32,
                             const float angleOffsetSamples = 0) const {
            constexpr float RADIUS = 10.0L;
            constexpr int16_t X_OFFSET = -100;
            constexpr int16_t Y_OFFSET = 100;
            const float angle = (sampleNumber - angleOffsetSamples) * PI / samplesPerCycle * 2.0f;
            return Coordinate{
                {
                    static_cast<int16_t>(X_OFFSET + round(sin(angle) * RADIUS)),
                    static_cast<int16_t>(Y_OFFSET + round(cos(angle) * RADIUS))
                }
            };
        }

        void expectFloatAreEqual(const float expected, const float actual, const char* description = "", const long index = 0) const {
            const float difference = fabsf(expected - actual);
            EXPECT_TRUE(difference < 0.002f) << description << ". expected: " <<  expected << " actual: " << actual << " difference: " << difference << " #" << index;
        }
        
        void expectFlowAnalysis(const FlowMeterDriver* actual, const char* description, const int index,
                                const float smoothX, const float smoothY, const float highPassX, const float highPassY,
                                const float distance, const float smoothDistance, const float angle) const {
            std::string message(description);
            message += std::string(" #") + std::to_string(index) + std::string(" @ ");
            expectFloatAreEqual(smoothX, actual->getSmoothSample().x, (message + "Smooth X").c_str(), index);
            expectFloatAreEqual(smoothY, actual->getSmoothSample().y, (message + "Smooth Y").c_str(), index);
            expectFloatAreEqual(highPassX, actual->getHighPassSample().x, (message + "High pass X").c_str(), index);
            expectFloatAreEqual(highPassY, actual->getHighPassSample().y, (message + "High pass Y").c_str(), index);
            expectFloatAreEqual(distance, actual->getAverageAbsoluteDistance(), (message + "AbsoluteDistance").c_str(), index);
            expectFloatAreEqual(smoothDistance, actual->getSmoothDistance(), (message + "SmoothDistance").c_str(), index);
            expectFloatAreEqual(angle, actual->getAngle(), (message + "angle").c_str(), index);
        }
    };

    EventServer FlowMeterTest::eventServer;

    TEST_F(FlowMeterTest, flowMeterFirstValueIsOutlierTest) {
        FlowMeterDriver actual(&eventServer);
        eventServer.subscribe(&actual, Topic::SensorWasReset);
        TestEventClient client(&eventServer);
        eventServer.subscribe(&client, Topic::ResetSensor);
        Coordinate sample{{3500, 3500}};
        actual.addSample(sample);
        expectResult(&actual, L"First measurement", 0, false, 0, true, false, false);
        EXPECT_EQ(4949.7474f, actual.getAverageAbsoluteDistance()) << "Absolute distance OK after first";
        sample.x = 1000;
        sample.y = 1000;
        for (int i = 0; i < 9; i++) {
            actual.addSample(sample);
            expectResult(&actual, L"Ignore outlier", i, false, 0, true, false, true);
            EXPECT_EQ(4949.7474f, actual.getAverageAbsoluteDistance()) << "Absolute distance not changed with outlier";
            EXPECT_EQ(0, client.getCallCount()) << "ResetSensor not published";
        }
        sample.x = 1020;
        sample.y = 1020;
        actual.addSample(sample);
        expectResult(&actual, L"Ignore outlier", 9, false, 0, true, false, true);
        EXPECT_EQ(1, client.getCallCount()) << "ResetSensor published";
        EXPECT_EQ(4949.7474f, actual.getAverageAbsoluteDistance()) << "Absolute distance still not changed with outlier";

        eventServer.publish(Topic::SensorWasReset, LONG_TRUE);
        // we're starting from scratch using the next value
        sample.x = 1500;
        sample.y = 1500;
        actual.addSample(sample);
        expectResult(&actual, L"first after outlier reset", 0, false, 0, true, false, false);
        EXPECT_EQ(2121.3203f, actual.getAverageAbsoluteDistance()) << "Absolute distance reset";
    }

    TEST_F(FlowMeterTest, flowMeterGoodFlowTest) {
        FlowMeterDriver flowMeter(&eventServer);
        flowMeter.begin(4, 390);
        expectFloatAreEqual(0.8604f, flowMeter.getZeroThreshold(), "Zero threshold");

        int totalPeaks = 0;
        bool inFlow = false;
        int flowSwitches = 0;
        int flowLength = 0;
        int longestFlow = 0;

        constexpr int STARTUP_IDLE_SAMPLES = 4;
        constexpr int SAMPLES_PER_CYCLE = 32;
        constexpr int SHUTDOWN_IDLE_SAMPLES = 88;
        constexpr int ANGLE_OFFSET_SAMPLES = SAMPLES_PER_CYCLE / 4;
        constexpr int CYCLES = 4;

        // smoothX, smoothY, highpassX, highpassY, averageAbsolutedistance, smoothDistance, angle
        constexpr float EXPECTED[][7] = {
            {-105.1087f, 94.4343f, -4.2356f, -4.0902f, 141.7776f, 7.6763f, -2.3737f}, // after cycle 1
            {-105.1048f, 94.4299f, -5.7681f, -3.6192f, 140.4442f, 7.0490f, -2.5812f}, // after cycle 2
            {-105.1048f, 94.4299f, -5.9877f, -3.5511f, 140.1859f, 6.9741f, -2.6063f}, // after cycle 3
            {-105.1048f, 94.4299f, -6.0191f, -3.5414f, 140.1359f, 6.9652f, -2.6098f}, // after cycle 4
        };

        Coordinate sample = getSample(0, SAMPLES_PER_CYCLE, ANGLE_OFFSET_SAMPLES);
        for (int i = 0; i < STARTUP_IDLE_SAMPLES; i++) {
            eventServer.publish(Topic::Sample, sample);
            if (i == 0) {
                expectFlowAnalysis(&flowMeter, "Init", i, -110.0f, 100.0f, 0, 0, 148.6607f, 0, -PI);
            }

            totalPeaks += flowMeter.isPeak();
        }
        expectFlowAnalysis(&flowMeter, "Init", STARTUP_IDLE_SAMPLES - 1, -110.0f, 100.0f, 0, 0, 148.6607f, 0, 0.7840f);
        expectResult(&flowMeter, L"After startup", 0, false, 0, true, false, false);
        EXPECT_EQ(0, totalPeaks) << "No peaks yet";
        EXPECT_FALSE(flowMeter.hasFlow()) << "No flow yet";
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
            const auto current = EXPECTED[cycle];
            expectFlowAnalysis(&flowMeter, "Cycle", cycle, current[0], current[1], current[2], current[3], current[4],
                               current[5], current[6]);
        }
        for (int i = 0; i < SHUTDOWN_IDLE_SAMPLES; i++) {
            eventServer.publish(Topic::Sample, sample);
        }
        expectFlowAnalysis(&flowMeter, "shutdown", SHUTDOWN_IDLE_SAMPLES, -110.0f, 98.0f, -0.0595f, 0.00578f, 147.2439f,
                           0.1347f, 3.04466f);
        expectResult(&flowMeter, L"Final", SHUTDOWN_IDLE_SAMPLES, false, 0, true, false, false);
        EXPECT_EQ(4, totalPeaks) << "Found 4 peaks";
        EXPECT_EQ(1, flowSwitches) << "Flow switched once";
        EXPECT_EQ(110, longestFlow) << "Longest flow is 110 samples";
    }

    TEST_F(FlowMeterTest, flowMeterOutlierTest) {
        // redirect std::cout so you can see it
        std::streambuf* backup = std::cout.rdbuf();
        const std::stringstream ss;
        std::cout.rdbuf(ss.rdbuf());

        FlowMeter flowMeter(&eventServer);
        flowMeter.begin(1, 390.0f);
        int totalOutliers = 0;
        Coordinate testData[] = {
            {{-81, -165}}, {{-80, -165}}, {{-82, -164}}, {{-80, -164}},
            {{-82, -164}}, {{-79, -165}}, {{-82, -165}}, {{-81, -165}},
            {{-82, -165}}, {{-81, -165}}, {{-82, -165}}, {{-79, -165}},
            {{-81, -164}}, {{-78, -161}}, {{-78, -159}}, {{-83, -158}},
            {{-68, -154}}, {{-80, -156}}, {{-67, -150}}, {{-75, -151}},
            {{-65, -146}}, {{-68, -148}}, {{-62, -143}}, {{-64, -148}},
            {{-59, -140}}, {{-55, -145}}, {{-53, -139}}, {{-46, -147}},
            {{-44, -139}}, {{-34, -150}}, {{-37, -145}}, {{-26, -157}}, {{27, 27}},
            {{-33, -153}}, {{-22, -168}}, {{-28, -164}}, {{-15, -182}},
            {{-30, -179}}, {{-22, -195}}, {{-38, -189}}, {{-25, -209}},
            {{-45, -200}}, {{-33, -215}}, {{-53, -203}}, {{-36, -221}},
            {{-61, -206}}, {{-45, -222}}, {{-67, -208}}, {{-47, -224}}, {{27, 27}},
            {{-77, -205}}, {{-54, -223}}, {{-80, -205}}, {{-60, -220}},
            {{-89, -200}}, {{-60, -219}}, {{-93, -197}}, {{-64, -216}},
            {{-93, -193}}, {{-68, -210}}, {{-100, -185}}, {{-67, -206}},
            {{-104, -184}}, {{-69, -203}}, {{-100, -178}}
        };
        for (const Coordinate entry : testData) {
            eventServer.publish(Topic::Sample, entry);
            totalOutliers += flowMeter.isOutlier();
            const auto smooth = flowMeter.getSmoothSample();
            std::cout << smooth.x << "," << smooth.y << (flowMeter.isExcluded() ? "E" : "I") << "\n";
        }
        EXPECT_EQ(2, totalOutliers) << "Found 3 outliers";

        // restore original buffer for cout
        std::cout.rdbuf(backup);
    }

    TEST_F(FlowMeterTest, flowMeterResetSensorTest) {
        FlowMeterDriver actual(&eventServer);
        actual.begin(4, 390.0f);
        EXPECT_TRUE(actual.wasReset()) << "was reset 1";
        for (int i = 0; i < 5; i++) {
            actual.addSample(getSample(static_cast<float>(i)));
        }
        expectFloatAreEqual(147.8443f, actual.getAverageAbsoluteDistance(), "Average absolute distance OK before");
        expectFloatAreEqual(-0.3364f, actual.getAngle(), "Angle OK before");
        expectFloatAreEqual(0.5748f, actual.getSmoothDistance(), "Smooth distance OK before");

        EXPECT_FALSE(actual.wasReset()) << "Not reset";
        actual.update(Topic::SensorWasReset, LONG_TRUE);
        EXPECT_TRUE(actual.wasReset()) << "was reset 2";
        actual.addSample(getSample(5));
        expectFloatAreEqual(140.3567f, actual.getAverageAbsoluteDistance(), "Average absolute distance OK after");
        expectFloatAreEqual(-3.1416f, actual.getAngle(), "Angle OK before");
        expectFloatAreEqual(0.0f, actual.getSmoothDistance(), "Smooth distance OK before");
    }

    TEST_F(FlowMeterTest, flowMeterSecondValueIsOutlierTest) {
        FlowMeterDriver actual(&eventServer);
        actual.begin(5, 390);
        actual.addSample(Coordinate{{3000, 2000}});
        expectResult(&actual, L"First measurement", 0, false, 0, true, false, false);
        expectFloatAreEqual(3605.5513f, actual.getAverageAbsoluteDistance(), "Absolute distance OK before");

        actual.addSample(Coordinate{{1500, 500}});
        // an outlier as second value should get ignored.
        expectResult(&actual, L"Early first outlier", 1, false, 0, true, false, true);
        expectFloatAreEqual(3605.5513f, actual.getAverageAbsoluteDistance(), "Absolute distance did not change with outlier");

        actual.addSample(Coordinate{{3025, 1980}});
        expectResult(&actual, L"Fist after outlier", 2, false, 0, true, false, false);
        expectFloatAreEqual(3606.04297f, actual.getAverageAbsoluteDistance(), "Absolute distance OK after");
    }

    TEST_F(FlowMeterTest, flowMeterResetPeakTest) {
        // If we have just found a peak and the next round flow goes off, the peak indicator must be reset
        FlowMeterDriver actual(&eventServer, {0.0,0.0}, { 0,0 }, 0, true, true, false, false);
        actual.begin(5, 390);
        actual.addSample(Coordinate{ {0, 0} });
        expectResult(&actual, L"First measurement", 0, false, 0, true, false, false);
    }
}
