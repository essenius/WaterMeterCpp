// Copyright 2022 Rik Essenius
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

#include <ESP.h>
#include "TestEventClient.h"
#include "../WaterMeterCpp/OledDriver.h"
#include "../WaterMeterCpp/EventServer.h"
#include "../WaterMeterCpp/ConnectionState.h"

namespace WaterMeterCppTest {
    
    class OledDriverTest : public testing::Test {
    public:
        static EventServer eventServer;

        static void publishConnectionState(ConnectionState connectionState) {
            eventServer.publish(Topic::Connection, static_cast<long>(connectionState));
        }
    };

    EventServer OledDriverTest::eventServer;
        // ReSharper disable once CyclomaticComplexity -- caused by EXPECT macros
        TEST_F(OledDriverTest, oledDriverCycleScriptTest) {
            Wire1.setFlatline(true); // make Wire1.endTransmission return the right value so begin() passes
            OledDriver oledDriver(&eventServer, &Wire1);
            TestEventClient client(&eventServer);
            eventServer.subscribe(&client, Topic::NoDisplayFound);
            const auto display = oledDriver.getDriver();
            display->sensorPresent(false);
            EXPECT_FALSE(oledDriver.begin()) << "No display";
            EXPECT_EQ(1, client.getCallCount()) << "No-display message sent";
            display->sensorPresent(true);
            client.reset();

            EXPECT_TRUE(oledDriver.begin()) << "Display found";
            EXPECT_EQ(0, display->getX()) << "cursor X=0";
            EXPECT_EQ(24, display->getY()) << "cursor Y=24";
            EXPECT_STREQ("Waiting", display->getMessage()) << "Message OK";
            EXPECT_EQ(0, client.getCallCount()) << "No messages";
            EXPECT_EQ(0u, oledDriver.display()) << "No need to display";

            publishConnectionState(ConnectionState::CheckFirmware);
            EXPECT_EQ(0b11110001, display->getFirstByte()) << "First byte of firmware logo ok";
            EXPECT_EQ(118, display->getX()) << "firmware X=118";
            EXPECT_EQ(0, display->getY()) << "firmware Y=0";
            EXPECT_EQ(25u, oledDriver.display()) << "display was needed";
            EXPECT_EQ(0u, oledDriver.display()) << "No need to display";

            const auto testValue = "23456.7890123";
            eventServer.publish(Topic::Volume, testValue);
            EXPECT_EQ(0, display->getX()) << "Meter X=0";
            EXPECT_EQ(8, display->getY()) << "Meter Y=8";
            EXPECT_STREQ("23456.7890123 m3 ", display->getMessage()) << "Meter message OK";

            eventServer.publish(Topic::Alert, LONG_TRUE);
            EXPECT_EQ(0b11111100, display->getFirstByte()) << "First byte of alert logo ok";
            EXPECT_EQ(108, display->getX()) << "alert on X=108";
            EXPECT_EQ(0, display->getY()) << "alert on Y=0";

            publishConnectionState(ConnectionState::WifiReady);
            EXPECT_EQ(0b01111000, display->getFirstByte()) << "First byte of wifi logo ok";
            EXPECT_EQ(118, display->getX()) << "Wifi X=118";
            EXPECT_EQ(0, display->getY()) << "Wifi Y=0";

            eventServer.publish(Topic::TimeOverrun, 1234567);
            EXPECT_EQ(0b00110000, display->getFirstByte()) << "First byte of time logo ok";
            EXPECT_EQ(108, display->getX()) << "Flow X=108";
            EXPECT_EQ(0, display->getY()) << "Flow Y=0";
            EXPECT_STREQ("Overrun:  1234567", display->getMessage()) << "Overrun message OK"; 

            eventServer.publish(Topic::TimeOverrun, 0);
            EXPECT_EQ(BLACK, display->getForegroundColor()) << "TimeOverrun off C=BLACK";
            EXPECT_EQ(108, display->getX()) << "Flow X=108";
            EXPECT_EQ(0, display->getY()) << "Flow Y=0";
            EXPECT_STREQ("Overrun:        0", display->getMessage()) << "Overrun off message OK"; 

            publishConnectionState(ConnectionState::MqttReady);
            EXPECT_EQ(0b00000000, display->getFirstByte()) << "First byte of mqtt logo ok";
            EXPECT_EQ(118, display->getX()) << "mqtt X=118";
            EXPECT_EQ(0, display->getY()) << "mqtt Y=0";

            eventServer.publish(Topic::Alert, LONG_FALSE);
            EXPECT_EQ(108, display->getX()) << "alert off X=108";
            EXPECT_EQ(0, display->getY()) << "alert off Y=0";
            EXPECT_EQ(7, display->getHeight()) << "alert off H=7";
            EXPECT_EQ(8, display->getWidth()) << "alert off W=8";
            EXPECT_EQ(BLACK, display->getForegroundColor()) << "Foreground is black";

            publishConnectionState(ConnectionState::RequestTime);
            EXPECT_EQ(0b00110000, display->getFirstByte()) << "First byte of time logo ok";
            EXPECT_EQ(118, display->getX()) << "Time X=118";
            EXPECT_EQ(0, display->getY()) << "Time Y=0";

            eventServer.publish(Topic::NoSensorFound, LONG_TRUE);
            EXPECT_EQ(0b00010011, display->getFirstByte()) << "First byte of missing sensor logo ok";
            EXPECT_EQ(98, display->getX()) << "missing sensor X=98";
            EXPECT_EQ(0, display->getY()) << "missing sensor Y=0";

            publishConnectionState(ConnectionState::Disconnected);
            EXPECT_EQ(118, display->getX()) << "disconnected X=118";
            EXPECT_EQ(0, display->getY()) << "disconnected Y=0";
            EXPECT_EQ(7, display->getHeight()) << "disconnected H=7";
            EXPECT_EQ(8, display->getWidth()) << "disconnected W=8";
            EXPECT_EQ(BLACK, display->getForegroundColor()) << "disconnected C=BLACK";

            eventServer.publish(Topic::SensorWasReset, LONG_TRUE);
            EXPECT_EQ(0b00010000, display->getFirstByte()) << "First byte of reset logo ok";
            EXPECT_EQ(108, display->getX()) << "reset X=108";
            EXPECT_EQ(0, display->getY()) << "reset Y=0";

            eventServer.publish(Topic::Pulses, 4321);
            EXPECT_EQ(0, display->getX()) << "Pulses X=0";
            EXPECT_EQ(0, display->getY()) << "Pulses Y=0";
            EXPECT_STREQ("Pulses:    4321", display->getMessage()) << "Pulses message OK";

            eventServer.publish(Topic::Blocked, LONG_TRUE);
            EXPECT_EQ(0b00111000, display->getFirstByte()) << "First byte of blocked logo ok";
            EXPECT_EQ(108, display->getX()) << "Blocked X=108";
            EXPECT_EQ(0, display->getY()) << "Blocked Y=0";

            eventServer.publish(Topic::UpdateProgress, 35);
            EXPECT_EQ(0, display->getX()) << "Progress X=0";
            EXPECT_EQ(16, display->getY()) << "Progress Y=16";
            EXPECT_STREQ("FW update: 35% ", display->getMessage()) << "Pulses message OK";

            eventServer.publish(Topic::NoFit, 35);
            EXPECT_EQ(0b00111001, display->getFirstByte()) << "First byte of NoFit logo ok";
            EXPECT_EQ(98, display->getX()) << "NoFit X=98";
            EXPECT_EQ(0, display->getY()) << "NoFit Y=0";
            EXPECT_STREQ("No fit:   35 deg ", display->getMessage()) << "NoFit message OK";

        }

    }
