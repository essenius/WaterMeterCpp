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
#include "../WaterMeterCpp/LedDriver.h"
#include "../WaterMeterCpp/Arduino.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {

	TEST_CLASS(LedDriverTest) {
	public:
		TEST_METHOD(LedDriverTest1) {

			LedDriver ledDriver;
			ledDriver.begin();
			Assert::AreEqual(OUTPUT, getPinMode(LED_BUILTIN), L"Built-in led on output");
			Assert::AreEqual(OUTPUT, getPinMode(ledDriver.RED_LED), L"Red led on output");
			Assert::AreEqual(OUTPUT, getPinMode(ledDriver.GREEN_LED), L"Green led on output");
			Assert::AreEqual(OUTPUT, getPinMode(ledDriver.BLUE_LED), L"Blue led on output");
			Assert::AreEqual(LOW, digitalRead(ledDriver.RED_LED), L"Red led off");
			Assert::AreEqual(LOW, digitalRead(ledDriver.GREEN_LED), L"Green led off");
			Assert::AreEqual(LOW, digitalRead(ledDriver.BLUE_LED), L"Blue led off");
			Assert::AreEqual(HIGH, digitalRead(LED_BUILTIN), L"Built-in led on");

			ledDriver.signalFlush(true);
			Assert::AreEqual(HIGH, digitalRead(ledDriver.BLUE_LED), L"Blue led on after flush");
			ledDriver.signalFlush(false);
			Assert::AreEqual(LOW, digitalRead(ledDriver.BLUE_LED), L"Blue led on after flush");

			ledDriver.signalConnected(true);
			Assert::AreEqual(HIGH, digitalRead(ledDriver.GREEN_LED), L"Green led on after connection");
			Assert::AreEqual(LOW, digitalRead(ledDriver.BLUE_LED), L"Blue led off after connection");

			ledDriver.signalError(true);
			Assert::AreEqual(HIGH, digitalRead(ledDriver.RED_LED), L"Red led on after error");

			ledDriver.signalError(false);
			Assert::AreEqual(LOW, digitalRead(ledDriver.RED_LED), L"Red led off after error reset");

			ledDriver.signalConnected(false);
			Assert::AreEqual(LOW, digitalRead(ledDriver.GREEN_LED), L"Green led off after disconnection");
			Assert::AreEqual(LOW, digitalRead(ledDriver.BLUE_LED), L"Blue led off after disconnection");

			AssertLedCycle(&ledDriver, true, false, ledDriver.EXCLUDE_INTERVAL, L"Exclude");
			AssertLedCycle(&ledDriver, false, true, ledDriver.FLOW_INTERVAL, L"Flow");
			AssertLedCycle(&ledDriver, false, false, ledDriver.WAIT_INTERVAL, L"Wait");
		}

	private:
		void AssertLedCycle(LedDriver *ledDriver, bool isExcluded, bool hasFlow, int interval, const wchar_t* description) {
			std::wstring messageBase(L"Built-in led for ");
			messageBase += std::wstring(description) + L" cycle " ;
			std::wstring messageOn = messageBase + L"ON";

			for (int i = 0; i < interval; i++) {
				ledDriver->signalMeasurement(isExcluded, hasFlow);
				AssertBuiltinLed(HIGH, messageOn.c_str(), i);
			}
			std::wstring messageOff = messageBase + L"OFF";
			for (int i = 0; i < interval; i++) {
				ledDriver->signalMeasurement(isExcluded, hasFlow);
				AssertBuiltinLed(LOW, messageOff.c_str(), i);
			}
			ledDriver->signalMeasurement(isExcluded, hasFlow);
			AssertBuiltinLed(HIGH, messageOn.c_str(), 255);
		}

		void AssertBuiltinLed(uint8_t expected, const wchar_t* description, int index) {
			std::wstring message(description);
			message += std::wstring(L" # ") + std::to_wstring(index);
			Assert::AreEqual(expected, digitalRead(LED_BUILTIN), message.c_str());
		}
	};
}
