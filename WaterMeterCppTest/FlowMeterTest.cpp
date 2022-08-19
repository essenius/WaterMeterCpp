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

#include <fstream>

#include "gtest/gtest.h"
#include "../WaterMeterCpp/FlowMeter.h"
#include "../WaterMeterCpp/EventServer.h"
#include "../WaterMeterCpp/EventClient.h"

#include "TestEventClient.h"
#include "FlowMeterDriver.h"
#include "PulseTestEventClient.h"

namespace WaterMeterCppTest {
    
    class FlowMeterTest : public testing::Test {
    public:
        static EventServer eventServer;
    protected:
        static constexpr float PI_F = 3.1415926536f;
        void expectResult(const FlowMeter* meter, const wchar_t* description, const int index,
                          const SearchTarget searchTarget = None, const bool pulse = false, const bool outlier = false) const {
            std::wstring message(description);
            message += std::wstring(L" #") + std::to_wstring(index) + std::wstring(L" @ ");
            EXPECT_EQ(searchTarget, meter->searchTarget()) << message << "Search target";
            EXPECT_EQ(pulse, meter->isPulse()) << message << "Pulse";
            EXPECT_EQ(outlier, meter->isOutlier()) << message << "Outlier";
        }

        Coordinate getSample(const float sampleNumber, const float samplesPerCycle = 32,
                             const float angleOffsetSamples = 0) const {
            constexpr float RADIUS = 10.0L;
            constexpr int16_t X_OFFSET = -100;
            constexpr int16_t Y_OFFSET = 100;
            const float angle = (sampleNumber - angleOffsetSamples) * PI_F / samplesPerCycle * 2.0f;
            return Coordinate{
                {
                    static_cast<int16_t>(X_OFFSET + round(sin(angle) * RADIUS)),
                    static_cast<int16_t>(Y_OFFSET + round(cos(angle) * RADIUS))
                }
            };
        }

        static void expectFloatAreEqual(const float expected, const float actual, const char* description = "", const long index = 0) {
            const float difference = fabsf(expected - actual);
            EXPECT_TRUE(difference < 0.002f) << description << ". expected: " <<  expected << " actual: " << actual << " difference: " << difference << " #" << index;
        }
        
        void expectFlowAnalysis(
            const FlowMeterDriver* actual,
            const char* message,
            const int index, 
            const FloatCoordinate movingAverage, 
            const FloatCoordinate smooth = { 0,0 }, 
            const FloatCoordinate smoothStartValue = { 0,0 },
            const bool flowStarted = false,
            const bool isPulse = false) const {
            EXPECT_FLOAT_EQ(movingAverage.x, actual->_movingAverage[index].x) << message << ": moving average X #" << index;
            EXPECT_FLOAT_EQ(movingAverage.y, actual->_movingAverage[index].y) << message << ": moving average Y #" << index;
            EXPECT_FLOAT_EQ(smooth.x, actual->_smooth.x) << message << ": smooth X";
            EXPECT_FLOAT_EQ(smooth.y, actual->_smooth.y) << message << ": smooth Y";
            EXPECT_FLOAT_EQ(smoothStartValue.x, actual->_smoothStartValue.x) << message << ": smooth start value X";
            EXPECT_FLOAT_EQ(smoothStartValue.y, actual->_smoothStartValue.y) << message << ": smooth start value Y";
            EXPECT_EQ(flowStarted, actual->_flowStarted) << ": flow started";
            EXPECT_EQ(isPulse, actual->_isPulse) << ": pulse detected";

        }

        // run process on test signals with a known number of pulses
        void flowTestWithFile(const char* fileName, const int maxXPulses = 0, const int maxYPulses = 0, const int minXPulses = 0, const int minYPulses = 0) const {
            FlowMeter flowMeter(&eventServer);
            const PulseTestEventClient pulseClient(&eventServer);
            flowMeter.begin(4, 390);
            Coordinate measurement{};
            std::ifstream measurements(fileName);
            EXPECT_TRUE(measurements.is_open()) << "File open";
            measurements.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            while (measurements >> measurement.x) {
                measurements >> measurement.y;
                eventServer.publish(Topic::Sample, measurement);
            }

           /* ASSERT_STREQ("", pulseClient.pulseHistory()); */

            ASSERT_EQ(maxXPulses, pulseClient.getExtremeCount(MaxX)) << "MaxX";
            ASSERT_EQ(maxYPulses, pulseClient.getExtremeCount(MaxY)) << "MaxY";
            ASSERT_EQ(minXPulses, pulseClient.getExtremeCount(MinX)) << "MinX";
            ASSERT_EQ(minYPulses, pulseClient.getExtremeCount(MinY)) << "MinY";
        }
    };

    EventServer FlowMeterTest::eventServer;

    TEST_F(FlowMeterTest, flowMeter0CyclesNoiseTest) {
        flowTestWithFile("0cyclesNoise.txt");
    }

    TEST_F(FlowMeterTest, flowMeter1CycleSlowTest) {
        flowTestWithFile("1cycleSlow.txt", 1,1,1, 2);
    }

    TEST_F(FlowMeterTest, flowMeter1CycleVerySlowTest) {
        flowTestWithFile("1cycleVerySlow.txt", 1,1,1,1);
    }

    TEST_F(FlowMeterTest, flowMeter1CycleSlowestTest) {
        flowTestWithFile("1cycleSlowest.txt", 1,1,1,1);
    }

    TEST_F(FlowMeterTest, flowMeter13CyclesSlowFastTest) {
        flowTestWithFile("13cyclesSlowFast.txt", 13,13,12,12);
    }

    TEST_F(FlowMeterTest, flowMeter35CyclesTest) {
        flowTestWithFile("35cycles.txt", 34,35,34,34);

    }

    TEST_F(FlowMeterTest, flowMeter77CyclesFastTest) {
        flowTestWithFile("77cyclesFast.txt", 76,77,77,76);
    }

    TEST_F(FlowMeterTest, flowMeterDetectPulseTest) {
        TestEventClient client(&eventServer);
        eventServer.subscribe(&client, Topic::Pulse);
        FlowMeterDriver actual(&eventServer);

        // jittering at the start to simulate standstill
        actual.begin(5, 390);

        actual.addSample({{100, 50}});
        expectFlowAnalysis(&actual, "No flow 1", 0, { 100, 50 });
        actual.detectPulse({{103, 53}});
        expectFlowAnalysis(&actual, "No flow 2", 1, { 103, 53 });
        actual.detectPulse({{105, 55}});
        expectFlowAnalysis(&actual, "No flow 3", 2, { 105, 55 });
        actual.detectPulse({{104, 54}});
        expectFlowAnalysis(&actual, "No flow 4", 3, { 104, 54 }, {103, 53}, {103, 53});
        actual.detectPulse({{102, 54}});
        expectFlowAnalysis(&actual, "No flow 5", 0, { 102, 54 },  { 103.25f, 53.5f }, { 103, 53 });

        // start moving. 

        actual.detectPulse({{80, 70}});
        expectFlowAnalysis(&actual, "Flow 1 (distance short)", 1, { 80, 70 }, { 100.5f, 55.875f }, { 103, 53 }, false);
        EXPECT_EQ(None, actual.searchTarget()) << "Not in search mode yet";
        EXPECT_EQ(nullptr, actual._currentSearcher) << "No searcher yet";
        actual.detectPulse({{70, 40}});

        expectFlowAnalysis(&actual, "Flow 2 (distance OK, angle wrong)", 2, { 70, 40 }, { 94.75, 55.1875f }, { 103, 53 }, false);
        EXPECT_EQ(None, actual.searchTarget()) << "Not in search mode yet";
        EXPECT_EQ(nullptr, actual._currentSearcher) << "No searcher yet";
        
        actual.detectPulse({{80, 69}});
        expectFlowAnalysis(&actual, "Flow 3 (detected)", 3, { 80, 69 }, { 88.875f, 56.71875f }, { 103, 53 }, true);
        EXPECT_EQ(MinX, actual.searchTarget()) << "MinX searcher selected (x going down, y going up)";

        // provide a minimal X

        actual.detectPulse({{60, 70}});
        actual.detectPulse({{60, 70}});
        actual.detectPulse({{60, 70}});
        actual.detectPulse({{65, 75}});
        actual.detectPulse({{70, 80}});
        actual.detectPulse({{75, 85}});
        EXPECT_EQ(MinX, actual.searchTarget()) << "Still searching for MinX";
        EXPECT_EQ(0, client.getCallCount());
        actual.detectPulse({{80, 90}});
        EXPECT_EQ(1, client.getCallCount()) << "Pulse reported for MinX";
        EXPECT_EQ(MaxY, actual.searchTarget()) << "MaxY searcher selected";

        // provide a maximal Y

        actual.detectPulse({{85, 95}});
        actual.detectPulse({{90, 100}});
        actual.detectPulse({{95, 105}});
        actual.detectPulse({{100, 100}});
        actual.detectPulse({{105, 95}});
        actual.detectPulse({{110, 90}});
        EXPECT_EQ(MaxY, actual.searchTarget()) << "Still searching for MaxY";
        EXPECT_EQ(1, client.getCallCount());
        actual.detectPulse({{115, 85}});
        EXPECT_EQ(2, client.getCallCount()) << "Pulse reported for MaxY";
        EXPECT_EQ(MaxX, actual.searchTarget()) << "MaxX searcher selected";

        // provide a maximal X

        actual.detectPulse({{120, 80}});
        actual.detectPulse({{115, 75}});
        actual.detectPulse({{110, 70}});
        actual.detectPulse({{105, 65}});
        EXPECT_EQ(MaxX, actual.searchTarget()) << "Still searching for MaxX";
        EXPECT_EQ(2, client.getCallCount());
        actual.detectPulse({{100, 60}});
        EXPECT_EQ(3, client.getCallCount()) << "Pulse reported for MaxX";
        EXPECT_EQ(MinY, actual.searchTarget()) << "MinY searcher selected";

        // provide a minimal Y

        actual.detectPulse({{95, 55}});
        actual.detectPulse({{90, 50}});
        actual.detectPulse({{85, 55}});
        actual.detectPulse({{80, 60}});
        actual.detectPulse({{75, 65}});
        EXPECT_EQ(MinY, actual.searchTarget()) << "Still searching for MinY";
        EXPECT_EQ(3, client.getCallCount());
        actual.detectPulse({{70, 70}});
        EXPECT_EQ(4, client.getCallCount()) << "Pulse reported for MinY";
        EXPECT_EQ(MinX, actual.searchTarget()) << "MinX searcher selected";
    }

/*    TEST_F(FlowMeterTest, flowMeterGetTargetTest) {
        FlowMeterDriver actual(&eventServer);
        ASSERT_EQ(MinX, actual.getTarget({ -5, 5 })) << "MinX target correctly identified";
        ASSERT_EQ(MaxY, actual.getTarget({ 0.01f, 2 })) << "MaxY target correctly identified";
        ASSERT_EQ(MinY, actual.getTarget({ -7, -2 })) << "MinY target correctly identified";
        ASSERT_EQ(MaxX, actual.getTarget({ 1, -2 })) << "MaxX target correctly identified";
    }*/

    TEST_F(FlowMeterTest, flowMeterSearcherTest) {
        FlowMeterDriver actual(&eventServer);
        ASSERT_EQ(MinX, actual.getSearcher(MinX)->target()) << "MinX searcher selected OK";
        ASSERT_EQ(MaxX, actual.getSearcher(MaxX)->target()) << "MaxX searcher selected OK";
        ASSERT_EQ(MinY, actual.getSearcher(MinY)->target()) << "MinY searcher selected OK";
        ASSERT_EQ(MaxY, actual.getSearcher(MaxY)->target()) << "MaxY searcher selected OK";
        ASSERT_EQ(nullptr, actual.getSearcher(None)) << "None selected nullptr";
    }

    TEST_F(FlowMeterTest, flowMeterFirstValueIsOutlierTest) {
        FlowMeterDriver actual(&eventServer);
        eventServer.subscribe(&actual, Topic::SensorWasReset);
        TestEventClient client(&eventServer);
        eventServer.subscribe(&client, Topic::ResetSensor);
        Coordinate sample{{3500, 3500}};
        actual.addSample(sample);
        expectResult(&actual, L"First measurement", 0);
        EXPECT_FLOAT_EQ(4949.7474f, actual._averageAbsoluteDistance) << "Absolute distance OK after first";
        sample.x = 1000;
        sample.y = 1000;
        for (int i = 0; i < static_cast<int>(FlowMeter::MAX_CONSECUTIVE_OUTLIERS - 1); i++) {
            actual.addSample(sample);
            expectResult(&actual, L"Ignore outlier", i, None, false, true);
            EXPECT_FLOAT_EQ(4949.7474f, actual._averageAbsoluteDistance) << "Absolute distance not changed with outlier";
            EXPECT_EQ(0, client.getCallCount()) << "ResetSensor not published";
        }
        sample.x = 1020;
        sample.y = 1020;
        actual.addSample(sample);
        expectResult(&actual, L"Ignore outlier", 9, None, false, true);
        EXPECT_EQ(1, client.getCallCount()) << "ResetSensor published";
        EXPECT_FLOAT_EQ(4949.7474f, actual._averageAbsoluteDistance) << "Absolute distance still not changed with outlier";

        eventServer.publish(Topic::SensorWasReset, LONG_TRUE);
        // we're starting from scratch using the next value
        sample.x = 1500;
        sample.y = 1500;
        actual.addSample(sample);
        expectResult(&actual, L"first after outlier begin", 0);
        EXPECT_EQ(2121.3203f, actual._averageAbsoluteDistance) << "Absolute distance begin";
    }

    TEST_F(FlowMeterTest, flowMeterOutlierTest) {
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
        }
        EXPECT_EQ(2, totalOutliers) << "Found 3 outliers";
    }

    TEST_F(FlowMeterTest, flowMeterResetSensorTest) {
        FlowMeterDriver actual(&eventServer);
        actual.begin(4, 390.0f);
        EXPECT_TRUE(actual.wasReset()) << "was reset at start";
        for (int i = 0; i < 5; i++) {
            actual.addSample(getSample(static_cast<float>(i)));
        }

        expectFlowAnalysis(&actual, "init", 1, { -98, 110 }, { -96.125f, 108.875f }, { -97, 109.25f }, false);

        EXPECT_FALSE(actual.wasReset()) << "No reset";
        actual.update(Topic::SensorWasReset, LONG_TRUE);
        EXPECT_TRUE(actual.wasReset()) << "was reset";
        actual.addSample(getSample(5));

        expectFlowAnalysis(&actual, "first sample after begin", 0, { -92, 106 });
    } 

    TEST_F(FlowMeterTest, flowMeterSecondValueIsOutlierTest) {
        FlowMeterDriver actual(&eventServer);
        actual.begin(5, 390);
        actual.addSample(Coordinate{{3000, 2000}});
        expectResult(&actual, L"First measurement", 0);
        expectFloatAreEqual(3605.5513f, actual._averageAbsoluteDistance, "Absolute distance OK before");

        actual.addSample(Coordinate{{1500, 500}});
        // an outlier as second value should get ignored.
        expectResult(&actual, L"Early first outlier", 1, None, false, true);
        expectFloatAreEqual(3605.5513f, actual._averageAbsoluteDistance, "Absolute distance did not change with outlier");

        actual.addSample(Coordinate{{3025, 1980}});
        expectResult(&actual, L"First after outlier", 2);
        expectFloatAreEqual(3606.04297f, actual._averageAbsoluteDistance, "Absolute distance OK after");
    }
}
