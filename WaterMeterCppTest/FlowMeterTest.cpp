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
#include "../WaterMeterCpp/FlowMeter.h"

#include <iostream>
#include <fstream>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
    TEST_CLASS(FlowMeterTest) {
    public:
        TEST_METHOD(flowMeterGoodFlowTest) {
            FlowMeter flowMeter;
            int measurement;
            float smoothValue;
            float derivative;
            float smoothDerivative;
            int totalPeaks = 0;
            std::ifstream measurements("flow_test.csv");
            Assert::IsTrue(measurements.is_open(), L"File open");
            long index = 0;
            while (measurements >> measurement) {
                flowMeter.addMeasurement(measurement);
                measurements >> smoothValue;
                assertFloatAreEqual(smoothValue, flowMeter.getSmoothValue(), L"Smooth value", index);
                measurements >> derivative;
                assertFloatAreEqual(derivative, flowMeter.getDerivative(), L"Derivative #", index);
                measurements >> smoothDerivative;
                assertFloatAreEqual(smoothDerivative, flowMeter.getSmoothDerivative(), L"Smooth Derivative", index);
                std::cout << measurement << "," << flowMeter.getSmoothValue() << "," << flowMeter.getDerivative() << ","
                    << flowMeter.getSmoothDerivative() << "," << flowMeter.isPeak() << "," << flowMeter.isExcluded() <<
                    "\n";
                totalPeaks += flowMeter.isPeak();
                index++;
            }
            Assert::AreEqual(5, totalPeaks, L"Found 5 peaks");
            measurements.close();
        }

        TEST_METHOD(flowMeterEarlyOutlierTest) {
            FlowMeter actual;
            actual.addMeasurement(5000);
            assertResult(&actual, L"Fist measurement", 5000.0f, 0.0f, 0.0f);

            actual.addMeasurement(2500);
            // an early outlier should result in a reset with the last value as seed.
            assertResult(&actual, L"Early first outlier",
                         2500.0f, 0.0f, 0.0f, false, true, true, true);

            // we're starting from scratch using the next value
            actual.addMeasurement(2510);
            assertResult(&actual, L"first after outlier reset", 2510.0f, 0.0f, 0.0f);
        }

        TEST_METHOD(flowMeterSingleOutlierIgnoredTest) {
            FlowMeter actual;
            actual.addMeasurement(1001);
            assertResult(&actual, L"First", 1001.0f, 0.0f, 0.0f);
            actual.addMeasurement(999);
            assertResult(&actual, L"Second", 1000.9f, -0.08f, -0.004f);
            actual.addMeasurement(1001);
            assertResult(&actual, L"Third", 1000.905f, -0.06f, -0.0068f);

            addMeasurementSeries(&actual, 7, [](const int i) { return 999 + i % 2 * 2; });
            assertResult(&actual, L"7 more good samples", 1000.5884f, -0.1547f, -0.043f);

            // introduce an outlier
            actual.addMeasurement(2000);
            assertResult(&actual, L"outlier ignored",
                         1000.5884f, -0.1547f, -0.043f, false, true, true);

            // new good data
            actual.addMeasurement(1001);
            assertResult(&actual, L"good value after outlier",
                         1000.6090f, -0.1073f, -0.046f);
        }

    private:
        void assertResult(FlowMeter* meter, const wchar_t* description,
                          const float smoothValue, const float derivative, const float smoothDerivative,
                          const bool peak = false, const bool excluded = false, const bool outlier = false,
                          const bool excludeAll = false) const {
            std::wstring message(description);
            message += std::wstring(L" @ ");

            assertFloatAreEqual(smoothValue, meter->getSmoothValue(), (message + L"Smooth Value").c_str());
            assertFloatAreEqual(derivative, meter->getDerivative(), (message + L"Derivative").c_str());
            assertFloatAreEqual(smoothDerivative, meter->getSmoothDerivative(),
                                (message + L"Smooth Derivative").c_str());
            Assert::AreEqual(peak, meter->isPeak(), (message + L"Peak").c_str());
            Assert::AreEqual(excluded, meter->isExcluded(), (message + L"Excluded").c_str());
            Assert::AreEqual(outlier, meter->isOutlier(), (message + L"Outlier").c_str());
            Assert::AreEqual(excludeAll, meter->areAllExcluded(), (message + L"All excluded").c_str());
        }

        static void addMeasurementSeries(FlowMeter* flowMeter, const int count, int (*f)(int)) {
            for (int i = 0; i < count; i++) {
                flowMeter->addMeasurement(f(i));
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
}
