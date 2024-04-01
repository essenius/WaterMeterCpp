// Copyright 2021-2024 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#include <fstream>
#include <MagnetoSensorNull.h>
#include <MagnetoSensorQmc.h>

#include "MagnetoSensorMock.h"
#include "gtest/gtest.h"

#include "TestEventClient.h"
#include "MagnetoSensorReaderDriver.h"
#include "MagnetoSensorSimulation.h"

// doesn't do much, just checking the interface works

namespace WaterMeterCppTest {
    using WaterMeter::SensorSample;
    using WaterMeter::MagnetoSensor;
    using WaterMeter::Topic;
    using WaterMeter::SensorState;

    TEST(MagnetoSensorReaderTest, readFailsTest) {
        EventServer eventServer;
        TestEventClient client(&eventServer);
        eventServer.subscribe(&client, Topic::SensorState);
        MagnetoSensorNull nullSensor;
        MagnetoSensor* list[] = {&nullSensor};
        digitalWrite(MagnetoSensorReader::DefaultPowerPort, LOW);
        MagnetoSensorReader sensorReader(&eventServer);
        EXPECT_FALSE(sensorReader.begin(list, 1)) << "Begin failed";
        const auto result = sensorReader.read();
        EXPECT_EQ(SensorState::ReadError, result.state()) << "Read has error";
    }

    TEST(MagnetoSensorReaderTest, customPowerPinTest) {
        constexpr int Pin = 23;
        digitalWrite(Pin, LOW);
        EventServer eventServer;
        MagnetoSensorNull nullSensor;
        MagnetoSensor* list[] = {&nullSensor};
        MagnetoSensorReaderDriver sensorReader(&eventServer);
        sensorReader.configurePowerPort(Pin);
        sensorReader.begin(list, 1);
        sensorReader.setPower(HIGH);
        EXPECT_EQ(HIGH, digitalRead(Pin)) << "Custom pin was toggled";
    }

    TEST(MagnetoSensorReaderTest, hardResetTest) {
        digitalWrite(MagnetoSensorReader::DefaultPowerPort, LOW);
        EventServer eventServer;
        TestEventClient client(&eventServer);
        eventServer.subscribe(&client, Topic::SensorState);
        MagnetoSensorNull nullSensor;
        MagnetoSensor* list[] = {&nullSensor};
        MagnetoSensorReaderDriver sensorReader(&eventServer);
        EXPECT_FALSE(sensorReader.hardReset()) << "Hard reset before init fails";

        sensorReader.begin(list, 1);
        EXPECT_EQ(1, client.getCallCount()) << "SensorState triggered";
        // simulate a power failure. The "off" should be triggered in reset as the state is now not right 
        digitalWrite(MagnetoSensorReader::DefaultPowerPort, LOW);
        EXPECT_TRUE(sensorReader.hardReset()) << "Hard reset succeeds";
        EXPECT_EQ(3, client.getCallCount()) << "SensorState triggered again (off and then on)";
        EXPECT_STREQ("3", client.getPayload()) << "State is BeginError";
        EXPECT_EQ(HIGH, digitalRead(MagnetoSensorReader::DefaultPowerPort)) << "Default Pin was toggled";
        sensorReader.setPower(HIGH);
        EXPECT_EQ(3, client.getCallCount()) << "SensorState not triggered again (not bypassed in setPower() but still error)";

        // emulate a reset in another process
        sensorReader._isHardResetting = true;
        EXPECT_FALSE(sensorReader.hardReset()) << "Hard reset when one is ongoing fails";

    }

    TEST(MagnetoSensorReaderTest, failedOnTest) {
        digitalWrite(MagnetoSensorReader::DefaultPowerPort, LOW);
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

    TEST(MagnetoSensorReaderTest, recoverTest) {
        digitalWrite(MagnetoSensorReader::DefaultPowerPort, LOW);
        EventServer eventServer;
        TestEventClient stateClient(&eventServer);
        eventServer.subscribe(&stateClient, Topic::SensorState);
        MagnetoSensorMock mockSensor;
        mockSensor.setBeginFailures(1);
        MagnetoSensor* list[] = {&mockSensor};
        MagnetoSensorReaderDriver sensorReader(&eventServer);
        EXPECT_FALSE(sensorReader.begin(list, 1)) << "Reader failed to start sensor";
        EXPECT_EQ(1, stateClient.getCallCount()) << "State sent once";
        EXPECT_STREQ("3", stateClient.getPayload()) << "Begin error";
        EXPECT_TRUE(sensorReader.begin(list, 1)) << "Reader started sensor";
        EXPECT_EQ(2, stateClient.getCallCount()) << "State sent twice";
        EXPECT_STREQ("1", stateClient.getPayload()) << "now OK";
    }

    TEST(MagnetoSensorReaderTest, softResetTest) {
        EventServer eventServer;
        TestEventClient client(&eventServer);
        eventServer.subscribe(&client, Topic::SensorState);
        MagnetoSensorNull nullSensor;
        MagnetoSensor* list[] = { &nullSensor };
        MagnetoSensorReaderDriver sensorReader(&eventServer);
        EXPECT_FALSE(sensorReader.softReset()) << "Soft reset before init fails";
        sensorReader.begin(list, 1);
        EXPECT_TRUE(sensorReader.softReset()) << "Soft reset after init succeeds";

        // emulate a reset in another process
        sensorReader._isHardResetting = true;
        EXPECT_TRUE(sensorReader.isResetting()) << "Resetting";
        EXPECT_FALSE(sensorReader.softReset()) << "Hard reset when one is ongoing fails";
        const auto result = sensorReader.read();
        EXPECT_EQ(SensorState::Resetting, result.state()) << "Can't read due to reset";
        sensorReader._isSoftResetting = true;
        EXPECT_TRUE(sensorReader.isResetting()) << "Still resetting";
        EXPECT_FALSE(sensorReader.softReset()) << "Hard reset when one is ongoing fails";
        sensorReader._isHardResetting = false;
        EXPECT_TRUE(sensorReader.isResetting()) << "And still resetting";
        EXPECT_FALSE(sensorReader.softReset()) << "Hard reset when one is ongoing fails";
        sensorReader._isSoftResetting = false;
        EXPECT_FALSE(sensorReader.isResetting()) << "Not resetting";

        EXPECT_TRUE(sensorReader.softReset()) << "Soft reset after succeeds again";
    }

    TEST(MagnetoSensorReaderTest, resetByStreakTest) {
        digitalWrite(MagnetoSensorReader::DefaultPowerPort, LOW);
        EventServer eventServer;
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

        constexpr SensorSample BaseSample{ {0,0} };
        for (int streaks = 0; streaks < 10; streaks++) {
            auto expectedCalls = streaks == 0 ? 0 : 1;
            stateClient.reset();
            for (int sample = 0; sample < 249; sample++) {
                EXPECT_EQ(SensorState::Ok, sensorReader.validate(BaseSample)) << "Validated OK for streak " << streaks << " sample " << sample;
                EXPECT_EQ(expectedCalls, stateClient.getCallCount()) << "right number of reset events fired for streak " << streaks << " sample " << sample;
            }
            EXPECT_EQ(streaks < 9 ? SensorState ::NeedsSoftReset : SensorState::NeedsHardReset, sensorReader.validate(BaseSample)) << "Validated OK for  streak " << streaks;
            EXPECT_EQ(expectedCalls + 1, stateClient.getCallCount()) << "ResetSensor request fired for streak " << streaks;
            EXPECT_STREQ(streaks < 9 ? "7" : "6", stateClient.getPayload()) << "Right reset request fired for streak " << streaks << " (7=soft, 6=hard)";
            if (streaks < 10) sensorReader.softReset(); else sensorReader.hardReset();
        }

        sensorReader.validate(BaseSample);
        EXPECT_EQ(3, stateClient.getCallCount()) << "ResetSensor request fired again after hard reset (to move to OK)";
        EXPECT_STREQ("1", stateClient.getPayload()) << "getState is OK afterwards";

        // start getting a signal

        EXPECT_EQ(SensorState::Ok, sensorReader.validate(SensorSample{ {1,1} })) << "getState OK after changed sensor value";

        stateClient.reset();
        eventServer.publish(Topic::ResetSensor, true);
        EXPECT_EQ(2, stateClient.getCallCount()) << "State changed twice after reset";
        EXPECT_STREQ("1", stateClient.getPayload()) << "getState is OK afterwards";
    }

    TEST(MagnetoSensorReaderTest, multipleResetTest) {
        EventServer eventServer;
        TestEventClient client(&eventServer);
        eventServer.subscribe(&client, Topic::SensorState);
        MagnetoSensorNull nullSensor;
        MagnetoSensor* list[] = { &nullSensor };
        MagnetoSensorReaderDriver sensorReader(&eventServer);
        sensorReader.begin(list, 1);
        sensorReader._isHardResetting = true;


    }
    TEST(MagnetoSensorReaderTest, dataWithResetTest) {
        EventServer eventServer;
        TestEventClient resetSensorClient(&eventServer);
        eventServer.subscribe(&resetSensorClient, Topic::SensorWasReset);
        MagnetoSensorSimulation sensor("manyresets.txt");
        MagnetoSensor* list[] = { &sensor };
        MagnetoSensorReaderDriver sensorReader(&eventServer);
        EXPECT_TRUE(sensorReader.begin(list, 1)) << "begin" ;
        bool endOfFile = false;
        int count = 0;
        int goodCount = 0;
        int badCount = 0;
        int otherCount = 0;
        while (!sensor.done()) {
            count++;
            SensorSample sample = sensorReader.read();
            std::cout << sample.x << ", " << sample.y << ": ";

            auto state = sensorReader.validate(sample);
            std::cout << sample.stateToString(state);
            switch (state) {
            case SensorState::ReadError:
                badCount++;
                break;
            case SensorState::Ok:
                goodCount++;
                break;
            default: {
                otherCount++;
            }
            }
            std::cout << std::endl;
        }
        EXPECT_EQ(0, resetSensorClient.getCallCount()) << "No reset events fired";
        EXPECT_EQ(2721, count) << "Count OK";
        EXPECT_EQ(2670, goodCount) << "Good count OK";
        EXPECT_EQ(51, badCount) << "Bad count OK";
        EXPECT_EQ(0, otherCount) << "Other count OK";
    }
}
