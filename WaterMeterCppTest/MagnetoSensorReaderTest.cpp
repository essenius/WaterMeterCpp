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

#include "TestEventClient.h"
#include "Wire.h"
#include "../WaterMeterCpp/MagnetoSensorReader.h"
#include "../WaterMeterCpp/MagnetoSensorNull.h"
#include "../WaterMeterCpp/MagnetoSensorQmc.h"

// doesn't do much, just checking the interface works

namespace WaterMeterCppTest {
    
    TEST(MagnetoSensorReaderTest, magnetoSensorReaderReadFailsTest) {
            constexpr int PIN = 23;
            digitalWrite(PIN, LOW);
            EventServer eventServer;
            TestEventClient client(&eventServer);
            eventServer.subscribe(&client, Topic::NoSensorFound);
            MagnetoSensorNull nullSensor;
            MagnetoSensor* list[] = {&nullSensor};
            MagnetoSensorReader sensorReader(&eventServer);
            sensorReader.begin(list, 1);
            const auto result = sensorReader.read();
            EXPECT_EQ(0, result.x) << "Read without sensor returns x=0";
            EXPECT_EQ(0, result.y) << "Read without sensor returns y=0";
        }

        TEST(MagnetoSensorReaderTest, magnetoSensorReaderCustomPowerPinTest) {
            constexpr int PIN = 23;
            digitalWrite(PIN, LOW);
            EventServer eventServer;
            MagnetoSensorNull nullSensor;
            MagnetoSensor* list[] = {&nullSensor};
            MagnetoSensorReader sensorReader(&eventServer);
            sensorReader.configurePowerPort(PIN);
            sensorReader.begin(list, 1);
            sensorReader.power(HIGH);
            EXPECT_EQ(HIGH, digitalRead(PIN)) << "Custom pin was toggled";
        }

        TEST(MagnetoSensorReaderTest, magnetoSensorReaderHardResetTest) {
            digitalWrite(MagnetoSensorReader::DEFAULT_POWER_PORT, LOW);
            EventServer eventServer;
            TestEventClient client(&eventServer);
            eventServer.subscribe(&client, Topic::NoSensorFound);
            MagnetoSensorNull nullSensor;
            MagnetoSensor* list[] = {&nullSensor};
            MagnetoSensorReader sensorReader(&eventServer);
            sensorReader.begin(list, 1);
            EXPECT_EQ(1, client.getCallCount()) << "NoSensorFound triggered";
            digitalWrite(MagnetoSensorReader::DEFAULT_POWER_PORT, LOW);
            sensorReader.hardReset();
            EXPECT_EQ(3, client.getCallCount()) << "NoSensorFound triggered again (off and then on)";
            EXPECT_EQ(HIGH, digitalRead(MagnetoSensorReader::DEFAULT_POWER_PORT)) << "Default Pin was toggled";
        }

        TEST(MagnetoSensorReaderTest, magnetoSensorReaderQmcTest1) {
            EventServer eventServer;
            TestEventClient resetSensorEventClient(&eventServer);
            TestEventClient alertEventClient(&eventServer);
            eventServer.subscribe(&resetSensorEventClient, Topic::SensorWasReset);
            eventServer.subscribe(&alertEventClient, Topic::Alert);
            MagnetoSensorReader reader(&eventServer);
            MagnetoSensorQmc qmcSensor;
            MagnetoSensor* list[] = {&qmcSensor};
            Wire.begin();

            EXPECT_TRUE(reader.begin(list, 1)) << "Reader found sensor";
            EXPECT_EQ(3000.0f, reader.getGain()) << "Gain = 3000";
            Wire.begin();
            Wire.setFlatline(true);
            Wire.setEndTransmissionTogglePeriod(10);

            for (int streaks = 0; streaks < 10; streaks++) {
                for (int sample = 0; sample < 249; sample++) {
                    reader.read();
                    EXPECT_EQ(streaks, resetSensorEventClient.getCallCount()) << "right number of events fired";
                    EXPECT_EQ(0, alertEventClient.getCallCount()) << "Alert event not fired";
                }
                reader.read();
                EXPECT_EQ(streaks + 1, resetSensorEventClient.getCallCount()) << "ResetSensor event fired";
            }
            reader.read();
            EXPECT_EQ(10, resetSensorEventClient.getCallCount()) << "ResetSensor event fired 10 times";
            EXPECT_EQ(1, alertEventClient.getCallCount()) << "Alert event fired";

            resetSensorEventClient.reset();
            // reset the test variables in Wire
            Wire.begin();
            eventServer.publish(Topic::ResetSensor, LONG_TRUE);
            EXPECT_EQ(1, resetSensorEventClient.getCallCount()) << "Sensor was reset called after ResetSensor command";
            Wire.setEndTransmissionTogglePeriod(0);
            // should contain the register set commands and 5x register 0 read
            constexpr uint8_t BUFFER[] = {10, 0x80, 11, 0x01, 9, 0x19, 0, 0, 0, 0, 0};
            EXPECT_EQ(sizeof BUFFER, Wire.writeMismatchIndex(BUFFER, sizeof BUFFER)) << "test";
        }
    }
