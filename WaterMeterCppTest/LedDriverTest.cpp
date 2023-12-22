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
    using WaterMeter::ConnectionState;
    using WaterMeter::EventServer;
    using WaterMeter::Led;
    using WaterMeter::LedDriver;
    using WaterMeter::Topic;

    class LedDriverTest : public testing::Test {
    public:
        static EventServer eventServer;
    protected:
        static void assertLed(const char* id, const uint8_t state, const unsigned char ledNo,
                              const char* description) {
            std::string message(description);
            message += std::string(": ") + id + std::string(" ") + std::to_string(state);
            EXPECT_EQ(state, Led::get(ledNo)) << message.c_str();
        }

        static void assertLeds(const uint8_t red, const uint8_t green, const uint8_t blue, const uint8_t aux, const uint8_t yellow,
                               const char* message) {
            assertLed("Red", red, Led::Red, message);
            assertLed("Green", green, Led::Green, message);
            assertLed("Blue", blue, Led::Blue, message);
            assertLed("Aux", aux, Led::Aux, message);
            assertLed("Yellow", yellow, Led::Yellow, message);
        }

        static void expectRunningLed(const uint8_t expected, const char* description, const unsigned int index) {
            std::string message(description);
            message += std::string(" # ") + std::to_string(index);
            EXPECT_EQ(expected, Led::get(Led::Running)) << message;
        }

        static void assertLedCycle(LedDriver* ledDriver, const Topic topic, const long payload, const int interval,
                                   const char* description) {
            std::string messageBase("Built-in led for ");
            messageBase += std::string(description) + " cycle ";
            const std::string messageOn = messageBase + "ON";
            ledDriver->update(topic, payload);

            // force a known state
            Led::set(Led::Running, Led::Off);
            for (int i = 0; i < interval; i++) {
                ledDriver->update(Topic::Sample, 508);
                expectRunningLed(Led::On, messageOn.c_str(), i);
            }
            const std::string messageOff = messageBase + "OFF";
            for (int i = 0; i < interval; i++) {
                ledDriver->update(Topic::Sample, 507);
                expectRunningLed(Led::Off, messageOff.c_str(), i);
            }
            ledDriver->update(Topic::Sample, 515);
            expectRunningLed(Led::On, messageOn.c_str(), 255);
        }

        static void publishConnectionState(const Topic topic, ConnectionState connectionState) {
            eventServer.publish(topic, static_cast<long>(connectionState));
        }
    };

    EventServer LedDriverTest::eventServer;

    TEST_F(LedDriverTest, ledDriverCycleInterruptTest) {
        LedDriver ledDriver(&eventServer);
        ledDriver.begin();
        Led::set(Led::Running, Led::Off);
        // go partway into a cycle
        for (unsigned int i = 0; i < LedDriver::ExcludeInterval / 5; i++) {
            eventServer.publish(Topic::Sample, 512);
            expectRunningLed(Led::On, "In first part", i);
        }
        // set a new state. Check whether it kicks in right away
        eventServer.publish(Topic::Anomaly, true);
        for (unsigned int i = 0; i < LedDriver::ExcludeInterval; i++) {
            eventServer.publish(Topic::Sample, 511);
            expectRunningLed(Led::On, "Started new cycle high", i);
        }
        for (unsigned int i = 0; i < LedDriver::ExcludeInterval; i++) {
            eventServer.publish(Topic::Sample, 510);
            expectRunningLed(Led::Off, "Started new cycle low", i);
        }
        // just into new cycle
        eventServer.publish(Topic::Sample, 513);
        expectRunningLed(Led::On, "Started second cycle high", 1);

        // ending flow. Check whether the cycle adapts
        eventServer.publish(Topic::Anomaly, false);
        for (unsigned int i = 0; i < LedDriver::IdleInterval; i++) {
            eventServer.publish(Topic::Sample, 514);
            expectRunningLed(Led::On, "Started new idle cycle high", i);
        }
        for (unsigned int i = 0; i < LedDriver::IdleInterval; i++) {
            eventServer.publish(Topic::Sample, 509);
            expectRunningLed(Led::Off, "Started new idle cycle high", i);
        }
    }

    TEST_F(LedDriverTest, ledDriverCycleTest) {
        LedDriver ledDriver(&eventServer);
        ledDriver.begin();
        assertLedCycle(&ledDriver, Topic::Anomaly, true, LedDriver::ExcludeInterval, "Anomaly");
        assertLedCycle(&ledDriver, Topic::Anomaly, false, LedDriver::IdleInterval, "Wait");
    }

    TEST_F(LedDriverTest, ledDriverTestEvents) {
        LedDriver ledDriver(&eventServer);
        ledDriver.begin();
        EXPECT_EQ(OUTPUT, getPinMode(Led::Running)) << "Built-in led on output";
        EXPECT_EQ(OUTPUT, getPinMode(Led::Red)) << "Red led on output";
        EXPECT_EQ(OUTPUT, getPinMode(Led::Green)) << "Green led on output";
        EXPECT_EQ(OUTPUT, getPinMode(Led::Blue)) << "Blue led on output";
        EXPECT_EQ(OUTPUT, getPinMode(Led::Aux)) << "Aux led on output";

        EXPECT_EQ(Led::Off, Led::get(Led::Running)) << "Built-in led off";

        assertLeds(Led::Off, Led::Off, Led::Off, Led::Off, Led::Off, "Initial values");

        publishConnectionState(Topic::Connection, ConnectionState::CheckFirmware);
        assertLeds(Led::Off, Led::Off, Led::On, Led::On, Led::Off, "Firmware check (blue on, aux blinking)");

        publishConnectionState(Topic::Connection, ConnectionState::MqttReady);
        assertLeds(Led::Off, Led::Off, Led::Off, Led::On, Led::Off, "Connected (blue off, aux on) ");

        eventServer.publish(Topic::Pulse, true);
        assertLeds(Led::Off, Led::Off, Led::On, Led::On, Led::Off, "Pulse (blue on)");

        eventServer.publish(Topic::ResultWritten, true);
        assertLeds(Led::Off, Led::Off, Led::Off, Led::On, Led::Off, "Result written (aux on, RGB off)");

        eventServer.publish(Topic::TimeOverrun, true);
        assertLeds(Led::On, Led::Off, Led::On, Led::On, Led::Off, "Overrun (red/blue on)");

        publishConnectionState(Topic::Connection, ConnectionState::Disconnected);
        assertLeds(Led::On, Led::Off, Led::Off, Led::Off, Led::Off, "Disconnected (aux off, blue off)");

        eventServer.publish(Topic::Pulse, false);
        assertLeds(Led::On, Led::Off, Led::Off, Led::Off, Led::Off, "No peak (blue stays off)");

        eventServer.publish(Topic::Blocked, false);
        assertLeds(Led::Off, Led::Off, Led::Off, Led::Off, Led::Off, "No more block (red off)");
        eventServer.publish(Topic::ConnectionError, "Problem");
        assertLeds(Led::On, Led::Off, Led::Off, Led::Off, Led::Off, "Error (red on)");

        eventServer.publish(Topic::Blocked, false);
        assertLeds(Led::Off, Led::Off, Led::Off, Led::Off, Led::Off, "No more block (red off)");

        eventServer.publish(Topic::Alert, true);
        assertLeds(Led::On, Led::On, Led::Off, Led::Off, Led::Off, "Alert (red and green on)");

        eventServer.publish(Topic::Alert, false);
        assertLeds(Led::Off, Led::Off, Led::Off, Led::Off, Led::Off, "No more block (red off)");
        eventServer.publish(Topic::SensorState, true);
        assertLeds(Led::On, Led::Off, Led::Off, Led::Off, Led::Off, "No sensor found (red on)");

        eventServer.publish(Topic::Alert, false); // switching red off again

        publishConnectionState(Topic::Connection, ConnectionState::Disconnected);

        for (unsigned int i = 0; i < LedDriver::ConnectingInterval; i++) {
            publishConnectionState(Topic::Connection, ConnectionState::WifiConnecting);
            assertLeds(Led::Off, Led::Off, Led::Off, Led::On, Led::Off,
                       ("looping ConnectingWifi ON - " + std::to_string(i)).c_str());
        }
        for (unsigned int i = 0; i < LedDriver::ConnectingInterval; i++) {
            publishConnectionState(Topic::Connection, ConnectionState::WifiConnecting);
            assertLeds(Led::Off, Led::Off, Led::Off, Led::Off, Led::Off,
                       ("looping ConnectingWifi OFF - " + std::to_string(i)).c_str());
        }
    }
}
