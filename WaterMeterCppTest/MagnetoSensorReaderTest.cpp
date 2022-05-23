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

#include "pch.h"

#include "CppUnitTest.h"
#include "TestEventClient.h"
#include "Wire.h"
#include "../WaterMeterCpp/MagnetoSensorReader.h"
#include "../WaterMeterCpp/MagnetoSensorNull.h"
#include "../WaterMeterCpp/MagnetoSensorQmc.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// doesn't do much, just checking the interface works

namespace WaterMeterCppTest {
    TEST_CLASS(MagnetoSensorReaderTest) {
    public:

        TEST_METHOD(magnetoSensorReaderReadFailsTest) {
            constexpr int PIN = 23;
            digitalWrite(PIN, LOW);
            EventServer eventServer;
            TestEventClient client(&eventServer);
            eventServer.subscribe(&client, Topic::NoSensorFound);
            MagnetoSensorNull nullSensor;
            MagnetoSensor* list[] = { &nullSensor };
            MagnetoSensorReader sensorReader(&eventServer);
            sensorReader.begin(list, 1);
            Assert::AreEqual<short>(0, sensorReader.read(), L"Read without sensor returns 0");

        }

        TEST_METHOD(magnetoSensorReaderCustomPowerPinTest) {
            constexpr int PIN = 23;
            digitalWrite(PIN, LOW);
            EventServer eventServer;
            MagnetoSensorNull nullSensor;
            MagnetoSensor* list[] = { &nullSensor };
            MagnetoSensorReader sensorReader(&eventServer);
            sensorReader.configurePowerPort(PIN);
            sensorReader.begin(list, 1);
            sensorReader.power(HIGH);
            Assert::AreEqual<int>(HIGH, digitalRead(PIN), L"Custom pin was toggled");
        }

        TEST_METHOD(magnetoSensorReaderHardResetTest) {
            digitalWrite(MagnetoSensorReader::DEFAULT_POWER_PORT, LOW);
            EventServer eventServer;
            TestEventClient client(&eventServer);
            eventServer.subscribe(&client, Topic::NoSensorFound);
            MagnetoSensorNull nullSensor;
            MagnetoSensor* list[] = { &nullSensor };
            MagnetoSensorReader sensorReader(&eventServer);
            sensorReader.begin(list, 1);
            Assert::AreEqual(1, client.getCallCount(), L"NoSensorFound triggered");
            digitalWrite(MagnetoSensorReader::DEFAULT_POWER_PORT, LOW);
            sensorReader.hardReset();
            Assert::AreEqual(3, client.getCallCount(), L"NoSensorFound triggered again (off and then on)");
            Assert::AreEqual<int>(HIGH, digitalRead(MagnetoSensorReader::DEFAULT_POWER_PORT), L"Default Pin was toggled");
        }

        TEST_METHOD(magnetoSensorReaderQmcTest1) {
            EventServer eventServer;
            TestEventClient resetSensorEventClient(&eventServer);
            TestEventClient alertEventClient(&eventServer);
            eventServer.subscribe(&resetSensorEventClient, Topic::SensorWasReset);
            eventServer.subscribe(&alertEventClient, Topic::Alert);
            MagnetoSensorReader reader(&eventServer);
            MagnetoSensorQmc qmcSensor;
            MagnetoSensor* list[] = { &qmcSensor };
            Wire.begin();

            Assert::IsTrue(reader.begin(list, 1), L"Reader found sensor");
            Assert::AreEqual(3000.0f, reader.getGain(), L"Gain = 3000");
            Wire.begin();
            Wire.setFlatline(true);
            Wire.setEndTransmissionTogglePeriod(10);

            for (int streaks = 0; streaks < 10; streaks++) {
                for (int sample = 0; sample < 250; sample++) {
                    reader.read();
                    Assert::AreEqual(streaks, resetSensorEventClient.getCallCount(), L"right number of events fired");
                    Assert::AreEqual(0, alertEventClient.getCallCount(), L"Alert event not fired");
                }
            }
            reader.read();
            Assert::AreEqual(10, resetSensorEventClient.getCallCount(), L"ResetSensor event fired 10 times");
            Assert::AreEqual(1, alertEventClient.getCallCount(), L"Alert event fired");

            resetSensorEventClient.reset();
            // reset the test variables in Wire
            Wire.begin();
            eventServer.publish(Topic::ResetSensor, LONG_TRUE);
            Assert::AreEqual(
                1,
                resetSensorEventClient.getCallCount(),
                L"Sensor was reset called after ResetSensor command");
            Wire.setEndTransmissionTogglePeriod(0);
            // should contain the register set commands and 5x register 0 read
            constexpr uint8_t BUFFER[] = {10, 0x80, 11, 0x01, 9, 0x19, 0, 0, 0, 0, 0};
            Assert::AreEqual<short>(sizeof BUFFER, Wire.writeMismatchIndex(BUFFER, sizeof BUFFER), L"test");
        }



    };
}
