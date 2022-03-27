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
        TEST_METHOD(magnetoSensorQmcAddressTest) {
            MagnetoSensor sensor;
            constexpr uint8_t ADDRESS = 0x23;
            sensor.configureAddress(ADDRESS);
            digitalWrite(MagnetoSensor::DEFAULT_POWER_PORT, LOW);
            sensor.begin();
            sensor.hardReset();
            Assert::AreEqual<int>(ADDRESS, Wire.getAddress(), L"Custom Address OK");
            Assert::AreEqual<int>(HIGH, digitalRead(MagnetoSensor::DEFAULT_POWER_PORT), L"Default Pin was toggled");
        }

        TEST_METHOD(magnetoSensorQmcHardResetTest) {
            MagnetoSensor sensor;
            constexpr uint8_t PIN = 23;
            sensor.configurePowerPort(PIN);
            digitalWrite(PIN, LOW);
            sensor.begin();
            sensor.hardReset();
            Assert::AreEqual<int>(HIGH, digitalRead(PIN), L"Custom pin was toggled");
            Assert::AreEqual<int>(MagnetoSensor::DEFAULT_ADDRESS, Wire.getAddress(), L"Default address OK");
        }

        TEST_METHOD(magnetoSensorQmcScriptTest) {
            MagnetoSensor sensor;
            sensor.begin();
            constexpr uint8_t BUFFER_BEGIN[] = {10, 0x80, 11, 0x01, 9, 0x19};
            Assert::AreEqual<int>(
                sizeof BUFFER_BEGIN,
                Wire.writeMismatchIndex(BUFFER_BEGIN, sizeof BUFFER_BEGIN),
                L"writes for begin ok");
            // we are at the default address so the sensor should report it's on
            Assert::IsTrue(sensor.isOn());
            // reset buffer
            Wire.begin();
            SensorData sample{};
            sensor.read(&sample);
            // the mock returns values from 0 increasing by 1 for every read
            Assert::AreEqual<int>(0x0100, sample.x, L"X ok");
            Assert::AreEqual<int>(0x0302, sample.y, L"Y ok");
            Assert::AreEqual<int>(0x0504, sample.z, L"Z ok");
            // last value we read was 5, so next will be 6. Last bit is 0 so data should not be reported ready
            Assert::IsFalse(sensor.dataReady(), L"Data not ready");
            // now 7, so that should report ready
            Assert::IsTrue(sensor.dataReady(), L"Data ready");
            constexpr uint8_t BUFFER_DATA_READY[] = {0, 6, 6};
            Assert::AreEqual<int>(
                sizeof BUFFER_DATA_READY,
                Wire.writeMismatchIndex(BUFFER_DATA_READY, sizeof BUFFER_DATA_READY),
                L"writes for read & data ready ok");
            // we configure another address so it reports off
            Wire.setEndTransmissionTogglePeriod(1);
            Assert::IsTrue(sensor.isOn(), L"Sensor is on");
            Assert::IsFalse(sensor.isOn(), L"Sensor is off");
            Wire.setEndTransmissionTogglePeriod(1);

            sensor.configureOverSampling(Sampling128);
            sensor.configureRate(Rate200Hz);
            sensor.configureRange(Range2G);
            sensor.begin();
            constexpr uint8_t BUFFER_RECONFIGURE[] = { 10, 0x80, 11, 0x01, 9, 0x8d };
            Assert::AreEqual<int>(
                sizeof BUFFER_RECONFIGURE,
                Wire.writeMismatchIndex(BUFFER_RECONFIGURE, sizeof BUFFER_RECONFIGURE),
                L"writes for reconfigure ok");

        }
    };
}
