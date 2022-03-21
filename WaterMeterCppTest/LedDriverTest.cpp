// Copyright 2021-2022 Rik Essenius
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
#include <ESP.h>

#include "../WaterMeterCpp/LedDriver.h"
#include "../WaterMeterCpp/EventServer.h"
#include "../WaterMeterCpp/ConnectionState.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {

    TEST_CLASS(LedDriverTest) {

    public:
        static EventServer eventServer;

        TEST_METHOD(ledDriverCycleInterruptTest) {
            LedDriver ledDriver(&eventServer);
            ledDriver.begin();
            Led::set(Led::RUNNING, Led::OFF);
            // go partway into a cycle
            for (unsigned int i = 0; i < LedDriver::EXCLUDE_INTERVAL / 5; i++) {
                eventServer.publish(Topic::Sample, 512);
                assertRunningLed(Led::ON, L"In first part", i);
            }
            // set a new state. Check whether it kicks in right away
            eventServer.publish(Topic::Flow, true);
            for (unsigned int i = 0; i < LedDriver::FLOW_INTERVAL; i++) {
                eventServer.publish(Topic::Sample, 511);
                assertRunningLed(Led::ON, L"Started new cycle high", i);
            }
            for (unsigned int i = 0; i < LedDriver::FLOW_INTERVAL; i++) {
                eventServer.publish(Topic::Sample, 510);
                assertRunningLed(Led::OFF, L"Started new cycle low", i);
            }
            // just into new cycle
            eventServer.publish(Topic::Sample, 513);
            assertRunningLed(Led::ON, L"Started second cycle high", 1);

            // ending flow. Check whether the cycle adapts
            eventServer.publish(Topic::Flow, false);
            for (unsigned int i = 0; i < LedDriver::IDLE_INTERVAL; i++) {
                eventServer.publish(Topic::Sample, 514);
                assertRunningLed(Led::ON, L"Started new idle cycle high", i);
            }
            for (unsigned int i = 0; i < LedDriver::IDLE_INTERVAL; i++) {
                eventServer.publish(Topic::Sample, 509);
                assertRunningLed(Led::OFF, L"Started new idle cycle high", i);
            }
        }

        TEST_METHOD(ledDriverCycleTest) {
            LedDriver ledDriver(&eventServer);
            ledDriver.begin();
            assertLedCycle(&ledDriver, Topic::Exclude, true, LedDriver::EXCLUDE_INTERVAL, L"Exclude");
            assertLedCycle(&ledDriver, Topic::Flow, true, LedDriver::FLOW_INTERVAL, L"Flow");
            assertLedCycle(&ledDriver, Topic::Exclude, false, LedDriver::IDLE_INTERVAL, L"Wait");
        }

        TEST_METHOD(ledDriverTestEvents) {
            LedDriver ledDriver(&eventServer);
            ledDriver.begin();
            Assert::AreEqual(OUTPUT, getPinMode(Led::RUNNING), L"Built-in led on output");
            Assert::AreEqual(OUTPUT, getPinMode(Led::RED), L"Red led on output");
            Assert::AreEqual(OUTPUT, getPinMode(Led::GREEN), L"Green led on output");
            Assert::AreEqual(OUTPUT, getPinMode(Led::BLUE), L"Blue led on output");
            Assert::AreEqual(OUTPUT, getPinMode(Led::AUX), L"Aux led on output");

            Assert::AreEqual(Led::OFF, Led::get(Led::RUNNING), L"Built-in led off");

            assertLeds(Led::OFF, Led::OFF, Led::OFF, Led::OFF, Led::OFF, L"Initial values");

            publishConnectionState(Topic::Connection, ConnectionState::CheckFirmware);
            assertLeds(Led::OFF, Led::OFF, Led::ON, Led::ON, Led::OFF, L"Firmware check (blue on, aux blinking)");

            publishConnectionState(Topic::Connection, ConnectionState::MqttReady);
            assertLeds(Led::OFF, Led::OFF, Led::OFF, Led::ON, Led::OFF, L"Connected (blue off, aux on) ");

            eventServer.publish(Topic::Peak, true);
            assertLeds(Led::OFF, Led::OFF, Led::ON, Led::ON, Led::OFF, L"Peak (blue on)");

            eventServer.publish(Topic::ResultWritten, true);
            assertLeds(Led::OFF, Led::OFF, Led::OFF, Led::ON, Led::OFF, L"Result written (aux on, RGB off)");

            eventServer.publish(Topic::TimeOverrun, true);
            assertLeds(Led::ON, Led::OFF, Led::ON, Led::ON, Led::OFF, L"Overrun (red/blue on)");

            publishConnectionState(Topic::Connection, ConnectionState::Disconnected);
            assertLeds(Led::ON, Led::OFF, Led::OFF, Led::OFF, Led::OFF, L"Disconnected (aux off, blue off)");

            eventServer.publish(Topic::Peak, false);
            assertLeds(Led::ON, Led::OFF, Led::OFF, Led::OFF, Led::OFF, L"No peak (blue stays off)");

            eventServer.publish(Topic::Blocked, LONG_FALSE);
            assertLeds(Led::OFF, Led::OFF, Led::OFF, Led::OFF, Led::OFF, L"No more block (red off)");

            eventServer.publish(Topic::ConnectionError, "Problem");
            assertLeds(Led::ON, Led::OFF, Led::OFF, Led::OFF, Led::OFF, L"Error (red on)");

            eventServer.publish(Topic::Blocked, LONG_FALSE);
            assertLeds(Led::OFF, Led::OFF, Led::OFF, Led::OFF, Led::OFF, L"No more block (red off)");

            eventServer.publish(Topic::Alert, LONG_TRUE);
            assertLeds(Led::ON, Led::ON, Led::OFF, Led::OFF, Led::OFF, L"Alert (red and green on)");

            eventServer.publish(Topic::Alert, LONG_FALSE);
            assertLeds(Led::OFF, Led::OFF, Led::OFF, Led::OFF, Led::OFF, L"No more block (red off)");

            publishConnectionState(Topic::Connection, ConnectionState::Disconnected);

            for (unsigned int i = 0; i < LedDriver::CONNECTING_INTERVAL; i++) {
                publishConnectionState(Topic::Connection, ConnectionState::WifiConnecting);
                assertLeds(Led::OFF, Led::OFF, Led::OFF, Led::ON, Led::OFF, (L"looping ConnectingWifi ON - " + std::to_wstring(i)).c_str());
            }
            for (unsigned int i = 0; i < LedDriver::CONNECTING_INTERVAL; i++) {
                publishConnectionState(Topic::Connection, ConnectionState::WifiConnecting);
                assertLeds(Led::OFF, Led::OFF, Led::OFF, Led::OFF, Led::OFF, (L"looping ConnectingWifi OFF - " + std::to_wstring(i)).c_str());
            }
        }

    private:
        void assertLed(const wchar_t* id, const uint8_t state, const unsigned char ledNo,
                       const wchar_t* description) const {
            std::wstring message(description);
            message += std::wstring(L": ") + id + std::wstring(L" ") + std::to_wstring(state);
            Assert::AreEqual(state, Led::get(ledNo), message.c_str());
        }

        void assertLeds(const uint8_t red, const uint8_t green, const uint8_t blue, const uint8_t aux, const uint8_t yellow,
                        const wchar_t* message) const {
            assertLed(L"Red", red, Led::RED, message);
            assertLed(L"Green", green, Led::GREEN, message);
            assertLed(L"Blue", blue, Led::BLUE, message);
            assertLed(L"Aux", aux, Led::AUX, message);
            assertLed(L"Yellow", yellow, Led::YELLOW, message);
        }

        void assertLedCycle(LedDriver* ledDriver, const Topic topic, const long payload, const int interval,
                            const wchar_t* description) const {
            std::wstring messageBase(L"Built-in led for ");
            messageBase += std::wstring(description) + L" cycle ";
            const std::wstring messageOn = messageBase + L"ON";
            ledDriver->update(topic, payload);

            // force a known state
            Led::set(Led::RUNNING, Led::OFF);
            for (int i = 0; i < interval; i++) {
                ledDriver->update(Topic::Sample, 508);
                assertRunningLed(Led::ON, messageOn.c_str(), i);
            }
            const std::wstring messageOff = messageBase + L"OFF";
            for (int i = 0; i < interval; i++) {
                ledDriver->update(Topic::Sample, 507);
                assertRunningLed(Led::OFF, messageOff.c_str(), i);
            }
            ledDriver->update(Topic::Sample, 515);
            assertRunningLed(Led::ON, messageOn.c_str(), 255);
        }

        void assertRunningLed(const uint8_t expected, const wchar_t* description, const unsigned int index) const {
            std::wstring message(description);
            message += std::wstring(L" # ") + std::to_wstring(index);
            Assert::AreEqual(expected, Led::get(Led::RUNNING), message.c_str());
        }

        static void publishConnectionState(const Topic topic, ConnectionState connectionState) {
            eventServer.publish(topic, static_cast<long>(connectionState));
        }
    };

    EventServer LedDriverTest::eventServer;
}
