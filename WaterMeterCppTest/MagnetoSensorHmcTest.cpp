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
#include "../WaterMeterCpp/MagnetoSensorHmc.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {

    TEST_CLASS(MagnetoSensorHmcTest) {
    public:
        TEST_METHOD(magnetoSensorHmcCustomAddressTest) {
            MagnetoSensorHmc sensor;
            constexpr uint8_t ADDRESS = 0x23;
            sensor.configureAddress(ADDRESS);
            Wire.begin();
            sensor.begin();
            sensor.hardReset();
            Assert::AreEqual<int>(ADDRESS, Wire.getAddress(), L"Custom Address OK");
        }

        TEST_METHOD(magnetoSensorHmcGetGainTest) {
            Assert::AreEqual(1370.0f, MagnetoSensorHmc::getGain(HmcGain0_88), L"0.88G gain ok");
            Assert::AreEqual(1090.0f, MagnetoSensorHmc::getGain(HmcGain1_3), L"1.3G gain ok");
            Assert::AreEqual(820.0f, MagnetoSensorHmc::getGain(HmcGain1_9), L"1.9G gain ok");
            Assert::AreEqual(660.0f, MagnetoSensorHmc::getGain(HmcGain2_5), L"2.5G gain ok");
            Assert::AreEqual(440.0f, MagnetoSensorHmc::getGain(HmcGain4_0), L"4.0G gain ok");
            Assert::AreEqual(390.0f, MagnetoSensorHmc::getGain(HmcGain4_7), L"4.7G gain ok");
            Assert::AreEqual(330.0f, MagnetoSensorHmc::getGain(HmcGain5_6), L"5.6G gain ok");
            Assert::AreEqual(230.0f, MagnetoSensorHmc::getGain(HmcGain8_1), L"8.1G gain ok");
        }

        TEST_METHOD(magnetoSensorHmcStandardAddressTest) {
            const MagnetoSensorHmc sensor;
            Wire.begin();
            Assert::AreEqual(390.0f,sensor.getGain(), L"Default gain OK");
            sensor.begin();
            sensor.hardReset();
            Assert::AreEqual<int>(MagnetoSensorHmc::DEFAULT_ADDRESS, Wire.getAddress(), L"Default address OK");
        }

        TEST_METHOD(magnetoSensorHmcTestTest) {
            const MagnetoSensorHmc sensor;
            Wire.begin();
            Assert::IsFalse(sensor.configure(), L"Sensor Test failed");
        }

        TEST_METHOD(magnetoSensorHmcTestInRangeTest) {
            SensorData sensorData{200, 400, 400, 1000};
            Assert::IsFalse(MagnetoSensorHmc::testInRange(&sensorData), L"Low X");
            sensorData.x = 600;
            Assert::IsFalse(MagnetoSensorHmc::testInRange(&sensorData), L"High X");
            sensorData.x = 400;
            Assert::IsTrue(MagnetoSensorHmc::testInRange(&sensorData), L"Within bounds");
            sensorData.y = 200;
            Assert::IsFalse(MagnetoSensorHmc::testInRange(&sensorData), L"Low Y");
            sensorData.y = 600;
            Assert::IsFalse(MagnetoSensorHmc::testInRange(&sensorData), L"High Y");
            sensorData.z = 200;
            Assert::IsFalse(MagnetoSensorHmc::testInRange(&sensorData), L"Low Z and high Y");
            sensorData.y = 400;
            Assert::IsFalse(MagnetoSensorHmc::testInRange(&sensorData), L"Low Z");
            sensorData.z = 600;
            Assert::IsFalse(MagnetoSensorHmc::testInRange(&sensorData), L"High Z");
        }

        TEST_METHOD(magnetoSensorHmcScriptTest) {
            MagnetoSensorHmc sensor;
            Wire.begin();
            sensor.begin();
            Assert::AreEqual<int>(MagnetoSensorHmc::DEFAULT_ADDRESS, Wire.getAddress(), L"Default address OK");
            constexpr uint8_t BUFFER_BEGIN[] = {0, 0x78, 1, 0xa0, 2, 0x01, 2, 0x01, 3};
            Assert::AreEqual<int>(
                sizeof BUFFER_BEGIN,
                Wire.writeMismatchIndex(BUFFER_BEGIN, sizeof BUFFER_BEGIN),
                L"writes for begin ok");
            // we are at the default address so the sensor should report it's on
            Wire.setEndTransmissionTogglePeriod(1);
            Assert::IsTrue(sensor.isOn(), L"Sensor is on");
            // reset buffer
            Wire.begin();
            SensorData sample{};
            sensor.read(&sample);
            // the mock returns values from 0 increasing by 1 for every read
            Assert::AreEqual<int>(0x001, sample.x, L"X ok");
            Assert::AreEqual<int>(0x0405, sample.y, L"Y ok");
            Assert::AreEqual<int>(0x0203, sample.z, L"Z ok");

            Wire.setEndTransmissionTogglePeriod(1);
            Assert::IsTrue(sensor.isOn(), L"Sensor is on");
            Assert::IsFalse(sensor.isOn(), L"Sensor is off");
            Wire.setEndTransmissionTogglePeriod(1);

            sensor.configureOverSampling(HmcSampling4); // 0b0100 0000
            sensor.configureRate(HmcRate30); // 0b0001 0100
            sensor.configureGain(HmcGain5_6); // 0b1100 0000

            // ensure all test variables are reset
            Wire.begin();
            sensor.begin();
            constexpr uint8_t BUFFER_RECONFIGURE[] = {0, 0x54, 1, 0xc0, 2, 0x01, 2, 0x01, 3};
            Assert::AreEqual<int>(
                sizeof BUFFER_RECONFIGURE,
                Wire.writeMismatchIndex(BUFFER_RECONFIGURE, sizeof BUFFER_RECONFIGURE),
                L"writes for reconfigure ok");
        }
    };
}
