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
#include "../WaterMeterCpp/Led.h"

#include "../WaterMeterCpp/LedDriver.h"
#include "../WaterMeterCpp/EventServer.h"
#include "../WaterMeterCpp/ConnectionState.h"

namespace WaterMeterCppTest {
    
    class LedDriverTest : public testing::Test {
    public:
        static EventServer eventServer;
    protected:
        void assertLed(const char* id, const uint8_t state, const unsigned char ledNo,
                       const char* description) const {
            std::string message(description);
            message += std::string(": ") + id + std::string(" ") + std::to_string(state);
            EXPECT_EQ(state, Led::get(ledNo)) << message.c_str();
        }

        void assertLeds(const uint8_t red, const uint8_t green, const uint8_t blue, const uint8_t aux, const uint8_t yellow,
                        const char* message) const {
            assertLed("Red", red, Led::RED, message);
            assertLed("Green", green, Led::GREEN, message);
            assertLed("Blue", blue, Led::BLUE, message);
            assertLed("Aux", aux, Led::AUX, message);
            assertLed("Yellow", yellow, Led::YELLOW, message);
        }

        void expectRunningLed(const uint8_t expected, const char* description, const unsigned int index) const {
            std::string message(description);
            message += std::string(" # ") + std::to_string(index);
            EXPECT_EQ(expected, Led::get(Led::RUNNING)) << message;
        }

        void assertLedCycle(LedDriver* ledDriver, const Topic topic, const long payload, const int interval,
                            const char* description) const {
            std::string messageBase("Built-in led for ");
            messageBase += std::string(description) + " cycle ";
            const std::string messageOn = messageBase + "ON";
            ledDriver->update(topic, payload);

            // force a known state
            Led::set(Led::RUNNING, Led::OFF);
            for (int i = 0; i < interval; i++) {
                ledDriver->update(Topic::Sample, 508);
                expectRunningLed(Led::ON, messageOn.c_str(), i);
            }
            const std::string messageOff = messageBase + "OFF";
            for (int i = 0; i < interval; i++) {
                ledDriver->update(Topic::Sample, 507);
                expectRunningLed(Led::OFF, messageOff.c_str(), i);
            }
            ledDriver->update(Topic::Sample, 515);
            expectRunningLed(Led::ON, messageOn.c_str(), 255);
        }

        static void publishConnectionState(const Topic topic, ConnectionState connectionState) {
            eventServer.publish(topic, static_cast<long>(connectionState));
        }
    };

    EventServer LedDriverTest::eventServer;

    TEST_F(LedDriverTest, ledDriverCycleInterruptTest) {
        LedDriver ledDriver(&eventServer);
        ledDriver.begin();
        Led::set(Led::RUNNING, Led::OFF);
        // go partway into a cycle
        for (unsigned int i = 0; i < LedDriver::EXCLUDE_INTERVAL / 5; i++) {
            eventServer.publish(Topic::Sample, 512);
            expectRunningLed(Led::ON, "In first part", i);
        }
        // set a new state. Check whether it kicks in right away
        eventServer.publish(Topic::Exclude, true);
        for (unsigned int i = 0; i < LedDriver::EXCLUDE_INTERVAL; i++) {
            eventServer.publish(Topic::Sample, 511);
            expectRunningLed(Led::ON, "Started new cycle high", i);
        }
        for (unsigned int i = 0; i < LedDriver::EXCLUDE_INTERVAL; i++) {
            eventServer.publish(Topic::Sample, 510);
            expectRunningLed(Led::OFF, "Started new cycle low", i);
        }
        // just into new cycle
        eventServer.publish(Topic::Sample, 513);
        expectRunningLed(Led::ON, "Started second cycle high", 1);

        // ending flow. Check whether the cycle adapts
        eventServer.publish(Topic::Exclude, false);
        for (unsigned int i = 0; i < LedDriver::IDLE_INTERVAL; i++) {
            eventServer.publish(Topic::Sample, 514);
            expectRunningLed(Led::ON, "Started new idle cycle high", i);
        }
        for (unsigned int i = 0; i < LedDriver::IDLE_INTERVAL; i++) {
            eventServer.publish(Topic::Sample, 509);
            expectRunningLed(Led::OFF, "Started new idle cycle high", i);
        }
    }

    TEST_F(LedDriverTest, ledDriverCycleTest) {
        LedDriver ledDriver(&eventServer);
        ledDriver.begin();
        assertLedCycle(&ledDriver, Topic::Exclude, true, LedDriver::EXCLUDE_INTERVAL, "Exclude");
        assertLedCycle(&ledDriver, Topic::Exclude, false, LedDriver::IDLE_INTERVAL, "Wait");
    }

    TEST_F(LedDriverTest, ledDriverTestEvents) {
        LedDriver ledDriver(&eventServer);
        ledDriver.begin();
        EXPECT_EQ(OUTPUT, getPinMode(Led::RUNNING)) << "Built-in led on output";
        EXPECT_EQ(OUTPUT, getPinMode(Led::RED)) << "Red led on output";
        EXPECT_EQ(OUTPUT, getPinMode(Led::GREEN)) << "Green led on output";
        EXPECT_EQ(OUTPUT, getPinMode(Led::BLUE)) << "Blue led on output";
        EXPECT_EQ(OUTPUT, getPinMode(Led::AUX)) << "Aux led on output";

        EXPECT_EQ(Led::OFF, Led::get(Led::RUNNING)) << "Built-in led off";

        assertLeds(Led::OFF, Led::OFF, Led::OFF, Led::OFF, Led::OFF, "Initial values");

        publishConnectionState(Topic::Connection, ConnectionState::CheckFirmware);
        assertLeds(Led::OFF, Led::OFF, Led::ON, Led::ON, Led::OFF, "Firmware check (blue on, aux blinking)");

        publishConnectionState(Topic::Connection, ConnectionState::MqttReady);
        assertLeds(Led::OFF, Led::OFF, Led::OFF, Led::ON, Led::OFF, "Connected (blue off, aux on) ");

        eventServer.publish(Topic::Pulse, true);
        assertLeds(Led::OFF, Led::OFF, Led::ON, Led::ON, Led::OFF, "Pulse (blue on)");

        eventServer.publish(Topic::ResultWritten, true);
        assertLeds(Led::OFF, Led::OFF, Led::OFF, Led::ON, Led::OFF, "Result written (aux on, RGB off)");

        eventServer.publish(Topic::TimeOverrun, true);
        assertLeds(Led::ON, Led::OFF, Led::ON, Led::ON, Led::OFF, "Overrun (red/blue on)");

        publishConnectionState(Topic::Connection, ConnectionState::Disconnected);
        assertLeds(Led::ON, Led::OFF, Led::OFF, Led::OFF, Led::OFF, "Disconnected (aux off, blue off)");

        eventServer.publish(Topic::Pulse, false);
        assertLeds(Led::ON, Led::OFF, Led::OFF, Led::OFF, Led::OFF, "No peak (blue stays off)");

        eventServer.publish(Topic::Blocked, LONG_FALSE);
        assertLeds(Led::OFF, Led::OFF, Led::OFF, Led::OFF, Led::OFF, "No more block (red off)");
        eventServer.publish(Topic::ConnectionError, "Problem");
        assertLeds(Led::ON, Led::OFF, Led::OFF, Led::OFF, Led::OFF, "Error (red on)");

        eventServer.publish(Topic::Blocked, LONG_FALSE);
        assertLeds(Led::OFF, Led::OFF, Led::OFF, Led::OFF, Led::OFF, "No more block (red off)");

        eventServer.publish(Topic::Alert, LONG_TRUE);
        assertLeds(Led::ON, Led::ON, Led::OFF, Led::OFF, Led::OFF, "Alert (red and green on)");

        eventServer.publish(Topic::Alert, LONG_FALSE);
        assertLeds(Led::OFF, Led::OFF, Led::OFF, Led::OFF, Led::OFF, "No more block (red off)");
        eventServer.publish(Topic::NoSensorFound, LONG_TRUE);
        assertLeds(Led::ON, Led::OFF, Led::OFF, Led::OFF, Led::OFF, "No sensor found (red on)");

        eventServer.publish(Topic::Alert, LONG_FALSE); // switching red off again

        publishConnectionState(Topic::Connection, ConnectionState::Disconnected);

        for (unsigned int i = 0; i < LedDriver::CONNECTING_INTERVAL; i++) {
            publishConnectionState(Topic::Connection, ConnectionState::WifiConnecting);
            assertLeds(Led::OFF, Led::OFF, Led::OFF, Led::ON, Led::OFF,
                       ("looping ConnectingWifi ON - " + std::to_string(i)).c_str());
        }
        for (unsigned int i = 0; i < LedDriver::CONNECTING_INTERVAL; i++) {
            publishConnectionState(Topic::Connection, ConnectionState::WifiConnecting);
            assertLeds(Led::OFF, Led::OFF, Led::OFF, Led::OFF, Led::OFF,
                       ("looping ConnectingWifi OFF - " + std::to_string(i)).c_str());
        }
    }
}
