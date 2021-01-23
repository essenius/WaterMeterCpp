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
#include "FlowMeterDriver.h"
#include "../WaterMeterCpp/FlowMeter.h"
#include <math.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
	TEST_CLASS(FlowMeterTest) {
	public:
		TEST_METHOD(FlowMeterDriftTest) {
			FlowMeterDriver actual;

			FlowMeterDriver expectedIdle(0.0f, false, false, 0.0f, 0.0f, false, 1000.0f, 1000.0f, 0.0f, 0.0f, false, false, false, false, 0 );
			assessAfterSeries(&expectedIdle, &actual, 10, [](int i) { return 1000; }, L"OK after 10 good samples to skip Startup, filters active");

			FlowMeterDriver expectedHighOutlier(970.0f, true, false, 0.0f, 0.0f, false, 1059.1f, 1190.0f, 130.9f,19.39f, true, true, false, false, 0);
			assessAfterSeries(&expectedHighOutlier, &actual, 3, [](int i) { return 2000; }, L"3 high outliers trigger Drift and Outlier");

			FlowMeterDriver expectedLowOutlier(1027.327f, true, false, 0.0f, 0.0f, false, 996.5072f, 963.9f, 32.6072f, 22.8972f, true, true, false, false, 0);
			assessAfterSeries(&expectedLowOutlier, &actual, 2, [](int i) { return 0; }, L"2 low outliers bring low pass results closer to mean");

			FlowMeterDriver expectedLagPeriod(1.8424f, false, false, 0.0f, 0.0f, false, 998.2129f, 996.44498f, 1.7679f, 8.0761f, true, true, false, false, 0);
			assessAfterSeries(&expectedLagPeriod, &actual, 22, [](int i) { return 1000; }, L"Waiting until last drift point...");

			FlowMeterDriver expectedFirstNormal(1.7871f, false, false, 0.0f, 0.0f, false, 998.2665f, 996.8005f, 1.4660f, 7.4151f, false, false, false, false, 0);
			assessAfterSeries(&expectedFirstNormal, &actual, 1, [](int i) { return 1000; }, L"No more drifting");
		}

		TEST_METHOD(FlowMeterEarlyOutlierTest) {
			FlowMeterDriver actual;

			actual.addMeasurement(5000);
			actual.addMeasurement(2500);
			FlowMeterDriver expectedFirstOutlier(2500.0f, true, true, 0.0f, 0.0f, false, 2500.0f, 2500.0f, 0.0f, 0.0f, false, true, true, false, 0);
			assertResultsAreEqual(&expectedFirstOutlier, &actual, L"Early first outlier");

			actual.addMeasurement(2510);
			FlowMeterDriver expectedFirstAfterOutlierReset(0.0f, false, false, 0.0f, 0.0f, false, 2510.0f, 2510.0f, 0.0f, 0.0f, false, false, false, false, 0);
			assertResultsAreEqual(&expectedFirstAfterOutlierReset, &actual, L"first after outlier reset");

			actual.addMeasurement(2490);
			FlowMeterDriver expectedSecondAfterOutlierReset(20.0f, false, false, -10.0f, 2.0f, false, 2509.4f, 2508.0f, 1.4f, 0.14f, false, false, false, false, 0);
			assertResultsAreEqual(&expectedSecondAfterOutlierReset, &actual, L"first after outlier reset");
		}

		TEST_METHOD(FlowMeterManyOutliersAfterStartupTest) {
			FlowMeterDriver actual;

			FlowMeterDriver expectedIdle(1.7870f, false, false, 0.6680f, 0.5842f, false, 999.2666f, 999.6856f, 0.4190f, 0.1811f, false, false, false, false, 0);
			assessAfterSeries(&expectedIdle, &actual, 10, [](int i) { return 999 + i % 2 * 2; }, L"10 good samples to skip Startup");

			FlowMeterDriver expectedFirstOutlier(1000.7334f, true, true, 0.6680f, 0.5842f, false, 999.2666f, 999.6856f, 0.4190f, 0.1811f, false, true, false, false, 0);
			assessAfterSeries(&expectedFirstOutlier, &actual, 1, [](int i) { return 2000; }, L"First outlier raises FirstOutlier, Outlier and Exclude");

			FlowMeterDriver expectedMoreOutliers(784.3182f, true, false, 0.6680f, 0.5842f, false, 1239.2114f, 1612.4577f, 373.2463f, 168.2699f, true, true, false, false, 0);
			assessAfterSeries(&expectedMoreOutliers, &actual, 9, [](int i) { return 2000; }, L"9 more outliers, high pass filter inactive, low pass work, drift on");
		}

		TEST_METHOD(FlowMeterStartFlowTest) {
			FlowMeterDriver actual;

			FlowMeterDriver expectedIdle(0, false, false, 0.0f, 0.0f, false, 2500.0f, 2500.0f, 0.0f, 0.0f, false, false, false, false, 0);
			assessAfterSeries(&expectedIdle, &actual, 2, [](int i) { return 2500; }, L"2 baseline samples");

			FlowMeterDriver expectedFlow(102.8306f, false, false, -67.1875f, 48.5583f, true, 2499.7456f, 2497.5339f, 2.2117f, 1.7764f, false, false, false, true, 0);
			assessAfterSeries(&expectedFlow, &actual, 6, [](int i) { return 2600 - i % 2 * 200; }, L"Flow of three vibrations");

			// we need the inaccuracy of 0.002 for these measurements. Difference and LowPassOnDifference are the only ones not meeting 0.001
			FlowMeterDriver expectedWait(0.1876f, false, false, 0.01602f, 5.1053f, false, 2499.8181f, 2499.2261f, 0.5919f, 1.2672f, false, false, false, false, 0);
			assessAfterSeries(&expectedWait, &actual, 11, [](int i) { return 2500; }, L"back to baseline");
		}

		TEST_METHOD(FlowMeterPeakTest) {
			FlowMeterDriver actual;
			addMeasurementSeries(&actual, 1, [](int i) { return 2500; });
			addMeasurementSeries(&actual, 8, [](int i) { return 2690 - (i % 2) * 380; });
			addMeasurementSeries(&actual, 10, [](int i) { return 2500; });

			FlowMeterDriver expectedFlow(0.4756f, false, false, 0.0616f, 13.47f, true, 2499.5386f, 2498.0142f, 1.5244f, 3.0785f, false, false, false, true, 0);
			assertResultsAreEqual(&expectedFlow, &actual, L"Just before peak");

			addMeasurementSeries(&actual, 1, [](int i) { return 2500; });

			FlowMeterDriver expectedPeak1(0.4614f, false, false, 0.0308f, 10.7826f, true, 2499.5524f, 2498.2127f, 1.3397f, 2.9046f, false, false, false, true, 1);
			assertResultsAreEqual(&expectedPeak1, &actual, L"At first peak");

			addMeasurementSeries(&actual, 1, [](int i) { return 2500; });
			Assert::AreEqual(0, actual.hasCalculatedPeak(), L"Just after first peak");
			Assert::AreEqual(0, actual.getPeak(), L"Just after first peak");

			addMeasurementSeries(&actual, 1, [](int i) { return 2690; });
			Assert::AreEqual(-1, actual.hasCalculatedPeak(), L"Start second vibration - calculated peak suppressed");
			Assert::AreEqual(0, actual.getPeak(), L"Start second vibration - calculated peak suppressed");

			addMeasurementSeries(&actual, 7, [](int i) { return 2310 + (i % 2) * 380; });
			Assert::AreEqual(0, actual.hasCalculatedPeak(), L"End of second vibration - delayed peak of first");
			Assert::AreEqual(-1, actual.getPeak(), L"End of second vibration - delayed peak of first");

			addMeasurementSeries(&actual, 1, [](int i) { return 2500; });
			Assert::AreEqual(0, actual.hasCalculatedPeak(), L"Just after second vibration");
			Assert::AreEqual(0, actual.getPeak(), L"Just after first peak");

			addMeasurementSeries(&actual, 7, [](int i) { return 2500; });
			Assert::AreEqual(1, actual.hasCalculatedPeak(), L"Calculated peak for second vibration");
			Assert::AreEqual(1, actual.getPeak(), L"Calculated peak for second vibration");

			addMeasurementSeries(&actual, 7, [](int i) { return 2500; });
			Assert::AreEqual(0, actual.hasCalculatedPeak(), L"No peak after horizon for second vibration");
			Assert::AreEqual(0, actual.getPeak(), L"No peak after horizon for second vibration");
		}

	private:
		static void addMeasurementSeries(FlowMeterDriver* flowMeter, int count, int (*f)(int)) {
			for (int i = 0; i < count; i++) {
				flowMeter->addMeasurement(f(i));
			}
		}
		static void assessAfterSeries(FlowMeterDriver* expected, FlowMeterDriver *actual, int count, int (*f)(int), const wchar_t* description) {
			addMeasurementSeries(actual, count, f);
			assertResultsAreEqual(expected, actual, description);
		}

		static void assertResultsAreEqual(FlowMeterDriver *expected, FlowMeterDriver *actual, const wchar_t *description) {
			std::wstring message(description);
			message += std::wstring(L" @ ");
			assertFloatAreEqual(expected->getAmplitude(), actual->getAmplitude(), (message + L"Amplitude").c_str());
			Assert::AreEqual(expected->isOutlier(), actual->isOutlier(), (message + L"Outlier").c_str());
			Assert::AreEqual(expected->isFirstOutlier(), actual->isFirstOutlier(), (message + L"FirstOutlier").c_str());
			assertFloatAreEqual(expected->getHighPass(), actual->getHighPass(), (message + L"HighPass").c_str());
			assertFloatAreEqual(expected->getLowPassOnHighPass(), actual->getLowPassOnHighPass(), (message + L"LowPassOnHighPass").c_str());
			Assert::AreEqual(expected->hasCalculatedFlow(), actual->hasCalculatedFlow(), (message + L"Calculated flow").c_str());
			assertFloatAreEqual(expected->getLowPassSlow(), actual->getLowPassSlow(), (message + L"LowPassSlow").c_str());
			assertFloatAreEqual(expected->getLowPassFast(), actual->getLowPassFast(), (message + L"LowPassFast").c_str());
			assertFloatAreEqual(expected->getLowPassDifference(), actual->getLowPassDifference(), (message + L"LowPassDifference").c_str());
			assertFloatAreEqual(expected->getLowPassOnDifference(), actual->getLowPassOnDifference(), (message + L"LowPassOnDifference").c_str());
			Assert::AreEqual(expected->hasDrift(), actual->hasDrift(), (message + L"Drift").c_str());
			Assert::AreEqual(expected->hasFlow(), actual->hasFlow(), (message + L"Flow").c_str());
			Assert::AreEqual(expected->isExcluded(), actual->isExcluded(), (message + L"Exclude").c_str());
			Assert::AreEqual(expected->areAllExcluded(), actual->areAllExcluded(), (message + L"ExcludeAll").c_str());
		}

		static void assertFloatAreEqual(float expected, float actual, const wchar_t* description = L"") {
			std::wstring message(description);
			float difference = fabsf(expected - actual);
			message += std::wstring(L". expected: ") + std::to_wstring(expected) + 
				       std::wstring(L" actual: " + std::to_wstring(actual) +
					   std::wstring(L" difference: ") + std::to_wstring(difference));
			Assert::IsTrue(difference < 0.002, message.c_str());
		}
	};
}
