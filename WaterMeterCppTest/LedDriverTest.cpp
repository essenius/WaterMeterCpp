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
#include "../WaterMeterCpp/ArduinoMock.h"
#include "../WaterMeterCpp/PubSub.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {

	TEST_CLASS(LedDriverTest) {
	public:
		TEST_METHOD(LedDriverTest1) {
			EventServer eventServer;
			LedDriver ledDriver(&eventServer);
			ledDriver.begin();
			Assert::AreEqual(OUTPUT, getPinMode(LED_BUILTIN), L"Built-in led on output");
			Assert::AreEqual(OUTPUT, getPinMode(LedDriver::RED_LED), L"Red led on output");
			Assert::AreEqual(OUTPUT, getPinMode(LedDriver::GREEN_LED), L"Green led on output");
			Assert::AreEqual(OUTPUT, getPinMode(LedDriver::BLUE_LED), L"Blue led on output");
			Assert::AreEqual(OUTPUT, getPinMode(LedDriver::AUX_LED), L"Aux led on output");
			Assert::AreEqual(HIGH, digitalRead(LED_BUILTIN), L"Built-in led on");

		    assertLeds(LOW, LOW, LOW, LOW,L"Initial values");

			eventServer.publish(Topic::Connected,  true);
			assertLeds(LOW, HIGH, LOW, LOW, L"Connected");
			eventServer.publish(Topic::Sending, true);
			assertLeds(LOW, HIGH, LOW, HIGH, L"Sending");
			eventServer.publish(Topic::TimeOverrun, true);
			assertLeds(HIGH, HIGH, LOW, HIGH, L"Overrun");
			eventServer.publish(Topic::Peak, true);
			assertLeds(HIGH, HIGH, HIGH, HIGH, L"Peak");
			eventServer.publish(Topic::Sending, false);
			assertLeds(HIGH, HIGH, HIGH, LOW, L"Stopped sending");
			eventServer.publish(Topic::TimeOverrun, false);
			assertLeds(LOW, HIGH, HIGH, LOW, L"No overrun");
			eventServer.publish(Topic::Disconnected, true);
			assertLeds(LOW, LOW, HIGH, LOW, L"Disconnected");
			eventServer.publish(Topic::Peak, false);
			assertLeds(LOW, LOW, LOW, LOW, L"No peak");
			eventServer.publish(Topic::Error, "Problem");
			assertLeds(HIGH, LOW, LOW, LOW, L"Error");
			eventServer.publish(Topic::Error, "");
			assertLeds(LOW, LOW, LOW, LOW, L"No error");


			AssertLedCycle(&ledDriver, Topic::Exclude, true, LedDriver::EXCLUDE_INTERVAL, L"Exclude");
			AssertLedCycle(&ledDriver, Topic::Exclude, false, LedDriver::WAIT_INTERVAL, L"Wait");
		}

	private:
		void assertLed(const wchar_t* id, uint8_t state, unsigned char ledNo, const wchar_t* description) {
			std::wstring message(description);
			message += std::wstring(L": ") + id + std::wstring(L" ") + std::to_wstring(state);
			Assert::AreEqual(state, digitalRead(ledNo), message.c_str());
		}

		void assertLeds(uint8_t red, uint8_t green, uint8_t blue, uint8_t aux, const wchar_t* message) {
			assertLed(L"Red", red, LedDriver::RED_LED, message);
			assertLed(L"Green", green, LedDriver::GREEN_LED, message);
			assertLed(L"Blue", blue, LedDriver::BLUE_LED, message);
			assertLed(L"Aux", aux, LedDriver::AUX_LED, message);
		}

		void AssertLedCycle(LedDriver *ledDriver, Topic topic, long payload, int interval, const wchar_t* description) {
			std::wstring messageBase(L"Built-in led for ");
			messageBase += std::wstring(description) + L" cycle " ;
			std::wstring messageOn = messageBase + L"ON";
			ledDriver->update(topic, payload);
			for (int i = 0; i < interval; i++) {
				ledDriver->update(Topic::Sample, "");
				AssertBuiltinLed(HIGH, messageOn.c_str(), i);
			}
			std::wstring messageOff = messageBase + L"OFF";
			for (int i = 0; i < interval; i++) {
				ledDriver->update(Topic::Sample, "");
				AssertBuiltinLed(LOW, messageOff.c_str(), i);
			}
			ledDriver->update(Topic::Sample, "");
			AssertBuiltinLed(HIGH, messageOn.c_str(), 255);
		}

		void AssertBuiltinLed(uint8_t expected, const wchar_t* description, int index) {
			std::wstring message(description);
			message += std::wstring(L" # ") + std::to_wstring(index);
			Assert::AreEqual(expected, digitalRead(LED_BUILTIN), message.c_str());
		}
	};
}
