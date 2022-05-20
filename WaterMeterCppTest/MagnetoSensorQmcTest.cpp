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

#include "pch.h"
#include "CppUnitTest.h"
#include "Wire.h"
#include "../WaterMeterCpp/MagnetoSensorQmc.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {

    TEST_CLASS(MagnetoSensorQmcTest) {
    public:

        TEST_METHOD(magnetoSensorQmcGetGainTest) {
            Assert::AreEqual(12000.0f, MagnetoSensorQmc::getGain(QmcRange2G), L"2G gain ok");
            Assert::AreEqual(3000.0f, MagnetoSensorQmc::getGain(QmcRange8G), L"8G gain ok");
        }

        TEST_METHOD(magnetoSensorQmcAddressTest) {
            MagnetoSensorQmc sensor;
            constexpr uint8_t ADDRESS = 0x23;
            sensor.configureAddress(ADDRESS);
            digitalWrite(MagnetoSensor::DEFAULT_POWER_PORT, LOW);
            Wire.begin();
            sensor.begin();
            sensor.hardReset();
            Assert::AreEqual<int>(ADDRESS, Wire.getAddress(), L"Custom Address OK");
            Assert::AreEqual<int>(HIGH, digitalRead(MagnetoSensor::DEFAULT_POWER_PORT), L"Default Pin was toggled");
        }

        TEST_METHOD(magnetoSensorQmcHardResetTest) {
            MagnetoSensorQmc sensor;
            constexpr uint8_t PIN = 23;
            sensor.configurePowerPort(PIN);
            digitalWrite(PIN, LOW);
            Wire.begin();
            sensor.begin();
            sensor.hardReset();
            Assert::AreEqual<int>(HIGH, digitalRead(PIN), L"Custom pin was toggled");
            Assert::AreEqual<int>(MagnetoSensorQmc::DEFAULT_ADDRESS, Wire.getAddress(), L"Default address OK");
        }

        TEST_METHOD(magnetoSensorQmcScriptTest) {
            MagnetoSensorQmc sensor;
            Wire.begin();
            sensor.begin();
            constexpr uint8_t BUFFER_BEGIN[] = {10, 0x80, 11, 0x01, 9, 0x19};
            Assert::AreEqual<int>(
                sizeof BUFFER_BEGIN,
                Wire.writeMismatchIndex(BUFFER_BEGIN, sizeof BUFFER_BEGIN),
                L"writes for begin ok");
            // we are at the default address so the sensor should report it's on
            Wire.setEndTransmissionTogglePeriod(1);
            Assert::IsTrue(sensor.isOn(), L"Sensor on");
            // reset buffer
            Wire.begin();
            SensorData sample{};
            sensor.read(&sample);
            // the mock returns values from 0 increasing by 1 for every read
            Assert::AreEqual<int>(0x0100, sample.x, L"X ok");
            Assert::AreEqual<int>(0x0302, sample.y, L"Y ok");
            Assert::AreEqual<int>(0x0504, sample.z, L"Z ok");

            // we configure another address so it reports off
            Wire.setEndTransmissionTogglePeriod(1);
            Assert::IsTrue(sensor.isOn(), L"Sensor is on");
            Assert::IsFalse(sensor.isOn(), L"Sensor is off");
            Wire.setEndTransmissionTogglePeriod(1);

            sensor.configureOverSampling(QmcSampling128);
            sensor.configureRate(QmcRate200Hz);
            sensor.configureRange(QmcRange2G);

            // ensure all test variables are reset
            Wire.begin();
            sensor.begin();
            constexpr uint8_t BUFFER_RECONFIGURE[] = { 10, 0x80, 11, 0x01, 9, 0x8d };
            Assert::AreEqual<int>(
                sizeof BUFFER_RECONFIGURE,
                Wire.writeMismatchIndex(BUFFER_RECONFIGURE, sizeof BUFFER_RECONFIGURE),
                L"writes for reconfigure ok");

        }
    };
}
