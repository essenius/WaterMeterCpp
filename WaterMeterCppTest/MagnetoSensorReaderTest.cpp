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

#include "MagnetoSensorMock.h"
#include "gtest/gtest.h"

#include "TestEventClient.h"
#include "Wire.h"
#include "MagnetoSensorReaderDriver.h"
#include "../WaterMeterCpp/MagnetoSensorNull.h"
#include "../WaterMeterCpp/MagnetoSensorQmc.h"

// doesn't do much, just checking the interface works

namespace WaterMeterCppTest {

    TEST(MagnetoSensorReaderTest, magnetoSensorReaderReadFailsTest) {
        EventServer eventServer;
        TestEventClient client(&eventServer);
        eventServer.subscribe(&client, Topic::SensorState);
        MagnetoSensorNull nullSensor;
        MagnetoSensor* list[] = {&nullSensor};
        digitalWrite(MagnetoSensorReader::DEFAULT_POWER_PORT, LOW);
        MagnetoSensorReader sensorReader(&eventServer);
        EXPECT_FALSE(sensorReader.begin(list, 1)) << "Begin failed";
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
        eventServer.subscribe(&client, Topic::SensorState);
        MagnetoSensorNull nullSensor;
        MagnetoSensor* list[] = {&nullSensor};
        MagnetoSensorReader sensorReader(&eventServer);
        sensorReader.begin(list, 1);
        EXPECT_EQ(1, client.getCallCount()) << "SensorState triggered";
        // simulate a power failure. The "off" should be triggered in reset as the state is now not right 
        digitalWrite(MagnetoSensorReader::DEFAULT_POWER_PORT, LOW);
        sensorReader.hardReset();
        EXPECT_EQ(3, client.getCallCount()) << "SensorState triggered again (off and then on)";
        EXPECT_STREQ("3", client.getPayload()) << "State is BeginError";
        EXPECT_EQ(HIGH, digitalRead(MagnetoSensorReader::DEFAULT_POWER_PORT)) << "Default Pin was toggled";
        sensorReader.power(HIGH);
        EXPECT_EQ(3, client.getCallCount()) << "SensorState not triggered again (not bypassed in power() but still error)";
    }

    TEST(MagnetoSensorReaderTest, magnetoSensorReaderFailedOnTest) {
        digitalWrite(MagnetoSensorReader::DEFAULT_POWER_PORT, LOW);
        EventServer eventServer;
        TestEventClient client(&eventServer);
        eventServer.subscribe(&client, Topic::Alert);
        MagnetoSensorMock mockSensor;
        mockSensor.setPowerOnFailures(3);
        MagnetoSensor* list[] = { &mockSensor };
        MagnetoSensorReaderDriver sensorReader(&eventServer);
        EXPECT_FALSE(sensorReader.begin(list, 1)) << "Reader found sensor but could not start";
        EXPECT_EQ(1, mockSensor.powerFailuresLeft()) << "had 2 failures (1 left)";
        EXPECT_TRUE(sensorReader.begin(list, 1)) << "Reader found sensor and succeeded start retry";

    }

    TEST(MagnetoSensorReaderTest, magnetoSensorReaderRecoverTest) {
        digitalWrite(MagnetoSensorReader::DEFAULT_POWER_PORT, LOW);
        EventServer eventServer;
        TestEventClient alertClient(&eventServer);
        eventServer.subscribe(&alertClient, Topic::Alert);
        TestEventClient stateClient(&eventServer);
        eventServer.subscribe(&stateClient, Topic::SensorState);
        MagnetoSensorMock mockSensor;
        mockSensor.setBeginFailures(1);
        MagnetoSensor* list[] = {&mockSensor};
        MagnetoSensorReaderDriver sensorReader(&eventServer);
        EXPECT_FALSE(sensorReader.begin(list, 1)) << "Reader failed to start sensor";
        EXPECT_EQ(1, alertClient.getCallCount()) << "Alert started (sensor not started)";
        EXPECT_EQ(1, stateClient.getCallCount()) << "State sent once";
        EXPECT_STREQ("3", stateClient.getPayload()) << "Begin error";
        EXPECT_TRUE(sensorReader.begin(list, 1)) << "Reader started sensor";
        EXPECT_EQ(1, alertClient.getCallCount()) << "Alert not stopped (even though sensor started)";
        EXPECT_EQ(2, stateClient.getCallCount()) << "State sent twice";
        EXPECT_STREQ("1", stateClient.getPayload()) << "now OK";
    }

    TEST(MagnetoSensorReaderTest, magnetoSensorReaderResetTest) {
        digitalWrite(MagnetoSensorReader::DEFAULT_POWER_PORT, LOW);
        EventServer eventServer;
        TestEventClient alertClient(&eventServer);
        eventServer.subscribe(&alertClient, Topic::Alert);
        TestEventClient stateClient(&eventServer);
        eventServer.subscribe(&stateClient, Topic::SensorState);
        TestEventClient resetSensorClient(&eventServer);
        eventServer.subscribe(&resetSensorClient, Topic::SensorWasReset);

        MagnetoSensorMock mockSensor;
        mockSensor.setFlat(true);
        MagnetoSensor* list[] = { &mockSensor };
        MagnetoSensorReaderDriver sensorReader(&eventServer);
        EXPECT_TRUE(sensorReader.begin(list, 1)) << "Reader succeeded to start sensor";
        EXPECT_DOUBLE_EQ(3000.0, sensorReader.getGain()) << "Gain OK";
        EXPECT_EQ(12, sensorReader.getNoiseRange()) << "Noise range OK";

        for (int streaks = 0; streaks < 10; streaks++) {
            for (int sample = 0; sample < 249; sample++) {
                sensorReader.read();
                EXPECT_EQ(streaks, resetSensorClient.getCallCount()) << "right number of reset events fired for streak " << streaks << " sample " << sample;
                EXPECT_EQ(0, alertClient.getCallCount()) << "Alert event not fired for streak " << streaks << " sample " << sample;
            }
            sensorReader.read();
            EXPECT_EQ(streaks + 1, resetSensorClient.getCallCount()) << "ResetSensor event fired for streak " << streaks;
        }
        EXPECT_EQ(1, alertClient.getCallCount()) << "Alert event fired";
        EXPECT_STREQ("1", alertClient.getPayload()) << "Payload is on";

        sensorReader.read();
        EXPECT_EQ(10, resetSensorClient.getCallCount()) << "ResetSensor event still fired 10 times";
        EXPECT_EQ(1, alertClient.getCallCount()) << "Alert event not fired again";

        // start getting a signal
        mockSensor.setFlat(false);
        sensorReader.read();
        EXPECT_EQ(2, alertClient.getCallCount()) << "Alert event fired";
        EXPECT_STREQ("0", alertClient.getPayload()) << "Payload is off";

        stateClient.reset();
        eventServer.publish(Topic::ResetSensor, true);
        EXPECT_EQ(2, stateClient.getCallCount()) << "State changed twice after reset";
    }
}
