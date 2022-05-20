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
#include <iostream>
#include <fstream>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
    TEST_CLASS(FlowMeterTest) {
    public:
        static EventServer eventServer;

        TEST_METHOD(flowMeterEarlyOutlierTest) {
            FlowMeter actual(&eventServer);
            actual.addSample(5000);
            assertResult(&actual, L"Fist measurement", 5000.0f, 0.0f, 0.0f);

            actual.addSample(2500);
            // an early outlier should result in a reset with the last value as seed.
            assertResult(&actual, L"Early first outlier",
                2500.0f, 0.0f, 0.0f, false, true, true, true);

            // we're starting from scratch using the next value
            actual.addSample(2510);
            assertResult(&actual, L"first after outlier reset", 2510.0f, 0.0f, 0.0f);
        }

        TEST_METHOD(flowMeterGoodFlowTest) {
            FlowMeter flowMeter(&eventServer);
            flowMeter.begin(4, 390);
            assertFloatAreEqual(0.8604f, flowMeter.getZeroThreshold(), L"Zero threshold", 0);
            int measurement;
            float fastSmoothValue;
            float fastDerivative;
            float smoothFastDerivative;
            float smoothAbsFastDerivative;
            float slowSmoothValue;
            float amplitude;
            float combinedDerivative;
            float smoothAbsCombinedDerivative;
            int totalPeaks = 0;
            std::ifstream measurements("unitTestFlow_4_7.csv");
            // y LPFY HPLPFy LPHPLPFy LPABSHPLPFy LPSY Ampl Comb AbsComb
            Assert::IsTrue(measurements.is_open(), L"File open");
            long index = 0;
            bool inFlow = false;
            int flowSwitches = 0;
            int flowLength = 0;
            int longestFlow = 0;
            int enteredBandCount = 0;
            while (measurements >> measurement) {
                eventServer.publish(Topic::Sample, measurement);
                measurements >> fastSmoothValue;
                assertFloatAreEqual(fastSmoothValue, flowMeter.getFastSmoothValue(), L"Fast Smooth #", index);
                measurements >> fastDerivative;
                assertFloatAreEqual(fastDerivative, flowMeter.getFastDerivative(), L"Fast Derivative #", index);
                measurements >> smoothFastDerivative;
                assertFloatAreEqual(smoothFastDerivative, flowMeter.getSmoothFastDerivative(), L"Smooth Fast Derivative", index);
                measurements >> smoothAbsFastDerivative;
                assertFloatAreEqual(smoothAbsFastDerivative, flowMeter.getSmoothAbsFastDerivative(), L"Smooth Abs Fast Derivative", index);
                measurements >> slowSmoothValue;
                assertFloatAreEqual(slowSmoothValue, flowMeter.getSlowSmoothValue(), L"Slow Smooth #", index);
                measurements >> amplitude;
                assertFloatAreEqual(amplitude, flowMeter.getAmplitude(), L"Amplitude #", index);
                measurements >> combinedDerivative;
                assertFloatAreEqual(combinedDerivative, flowMeter.getCombinedDerivative(), L"Combined Derivative", index);
                measurements >> smoothAbsCombinedDerivative;
                assertFloatAreEqual(smoothAbsCombinedDerivative, flowMeter.getSmoothAbsCombinedDerivative(), L"Smooth Abs Combined Derivative", index);
                std::cout << measurement << "," << flowMeter.getFastDerivative() << "," << flowMeter.getSmoothFastDerivative() << "," << flowMeter.getCombinedDerivative() << "," << flowMeter.isPeak() << "," << flowMeter.isExcluded() <<
                    "\n";
                totalPeaks += flowMeter.isPeak();
                enteredBandCount += flowMeter.hasEnteredBand();
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
                 index++;
            }
            Assert::AreEqual(3000L, index, L"3000 samples");
            Assert::AreEqual(13, totalPeaks, L"Found 13 peaks");
            Assert::AreEqual(20, flowSwitches, L"Flow switched 20 times");
            Assert::AreEqual(840, longestFlow, L"Longest flow is 840 samples");
            Assert::AreEqual(403, enteredBandCount, L"Entered band count is 403");
            measurements.close();
        }

        TEST_METHOD(flowMeterOutlierTest) {
            // redirect std::cout so you can see it
            std::streambuf* backup = std::cout.rdbuf();
            const std::stringstream ss;
            std::cout.rdbuf(ss.rdbuf());

            FlowMeter flowMeter(&eventServer);
            flowMeter.begin(1, 390.0f);
            int measurement;
            int totalOutliers = 0;
            std::ifstream measurements("outliertest.txt");
            Assert::IsTrue(measurements.is_open(), L"File open");
            long index = 0;
            while (measurements >> measurement) {
                std::cout << measurement << ",";
                eventServer.publish(Topic::Sample, measurement);
                std::cout << flowMeter.getFastSmoothValue() << "," << flowMeter.getFastDerivative() << ","
                    << flowMeter.getSmoothFastDerivative() << "," << flowMeter.isPeak() << "," << flowMeter.isExcluded() <<
                    "\n";
                Logger::WriteMessage(ss.str().c_str());
                totalOutliers += flowMeter.isOutlier();
                index++;
                measurements.ignore(std::numeric_limits<std::streamsize>::max(), ',');
            }
            Assert::AreEqual(3, totalOutliers, L"Found 3 outliers");
            measurements.close();

            // restore original buffer for cout
            std::cout.rdbuf(backup);
        }

        TEST_METHOD(flowMeterResetSensorTest) {
            FlowMeter actual(&eventServer);
            actual.begin(4, 390.0f);
            Assert::IsTrue(actual.wasReset(), L"was reset");
            addMeasurementSeries(&actual, 5, [](const int i) { return 180 + i % 2 * 2; });
            assertFloatAreEqual(0.4737f, actual.getSmoothFastDerivative());
            Assert::IsFalse(actual.wasReset(), L"Not reset");
            actual.update(Topic::SensorWasReset, LONG_TRUE);
            actual.addSample(175);
            Assert::IsTrue(actual.wasReset(), L"was reset");
            assertFloatAreEqual(175.0f, actual.getFastSmoothValue(), L"Fast Smooth");
            assertFloatAreEqual(0.0f, actual.getFastDerivative(), L"Fast Derivative");
            assertFloatAreEqual(0.0f, actual.getSmoothFastDerivative(), L"Fast Smooth Derivative");
            assertFloatAreEqual(0, actual.getSmoothAbsFastDerivative(), L"Smooth Abs Fast Derivative");
            assertFloatAreEqual(175.0f, actual.getSlowSmoothValue(), L"Slow Smooth");
            assertFloatAreEqual(0.0f, actual.getCombinedDerivative(), L"Combined Derivative");
            assertFloatAreEqual(0.0f, actual.getSmoothAbsCombinedDerivative(), L"Smooth Abs Combined Derivative");
        }

        TEST_METHOD(flowMeterSingleOutlierIgnoredTest) {
            FlowMeter actual(&eventServer);
            actual.begin(4, 390.0f);
            actual.addSample(1001);
            assertResult(&actual, L"First", 1001.0f, 0.0f, 0.0f);
            actual.addSample(999);
            assertResult(&actual, L"Second", 1000.33f, -0.6305f, -0.2112f);
            actual.addSample(1001);
            assertResult(&actual, L"Third", 1000.554f, -0.3821f, -0.2684f);

            addMeasurementSeries(&actual, 7, [](const int i) { return 999 + i % 2 * 2; });
            assertResult(&actual, L"7 more good samples", 999.8191f, -0.8125f, -0.6699f);

            // introduce an outlier
            actual.addSample(2000);
            assertResult(&actual, L"outlier ignored", 999.8191f, -0.8125f, -0.6699f, false, true, true);

            // new good data
            actual.addSample(1001);
            assertResult(&actual, L"good value after outlier", 1000.215f, -0.3923f, -0.577f);
        }

    private:
        void assertResult(const FlowMeter* meter, const wchar_t* description,
                          const float smoothValue, const float derivative, const float smoothDerivative,
                          const bool peak = false, const bool excluded = false, const bool outlier = false,
                          const bool excludeAll = false) const {
            std::wstring message(description);
            message += std::wstring(L" @ ");

            assertFloatAreEqual(smoothValue, meter->getFastSmoothValue(), (message + L"Smooth Value").c_str());
            assertFloatAreEqual(derivative, meter->getFastDerivative(), (message + L"Derivative").c_str());
            assertFloatAreEqual(smoothDerivative, meter->getSmoothFastDerivative(),
                                (message + L"Smooth Derivative").c_str());
            Assert::AreEqual(peak, meter->isPeak(), (message + L"Peak").c_str());
            Assert::AreEqual(excluded, meter->isExcluded(), (message + L"Excluded").c_str());
            Assert::AreEqual(outlier, meter->isOutlier(), (message + L"Outlier").c_str());
            Assert::AreEqual(excludeAll, meter->areAllExcluded(), (message + L"All excluded").c_str());
        }

        static void addMeasurementSeries(FlowMeter* flowMeter, const int count, int (*f)(int)) {
            for (int i = 0; i < count; i++) {
                flowMeter->addSample(f(i));
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
