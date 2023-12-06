// Copyright 2022-2023 Rik Essenius
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
#include "Wire.h"
#include "../WaterMeterCpp/MagnetoSensorQmc.h"
#include "../WaterMeterCpp/MagnetoSensorReader.h"

namespace WaterMeterCppTest {

    TEST(MagnetoSensorQmcTest, magnetoSensorQmcGetGainTest) {
        EXPECT_EQ(12000.0, MagnetoSensorQmc::getGain(QmcRange2G)) << "2G gain ok";
        EXPECT_EQ(3000.0, MagnetoSensorQmc::getGain(QmcRange8G)) << "8G gain ok";
    }

    TEST(MagnetoSensorQmcTest, magnetoSensorQmcAddressTest) {
        MagnetoSensorQmc sensor;
        constexpr uint8_t Address = 0x23;
        sensor.configureAddress(Address);
        Wire.begin();
        sensor.begin();
        EXPECT_EQ(Address, Wire.getAddress()) << "Custom Address OK";
        // additional tests now we have an instance

        EXPECT_TRUE(sensor.isReal()) << "Sensor is real";
        EXPECT_EQ(3000.0, sensor.getGain()) << "default gain ok";

    }

    TEST(MagnetoSensorQmcTest, magnetoSensorQmcDefaultAddressTest) {
        MagnetoSensorQmc sensor;
        Wire.begin();
        sensor.begin();
        EXPECT_EQ(MagnetoSensorQmc::DefaultAddress, Wire.getAddress()) << "Default address OK";
    }

    TEST(MagnetoSensorQmcTest, magnetoSensorQmcScriptTest) {
        MagnetoSensorQmc sensor;
        Wire.begin();
        sensor.begin();
        constexpr uint8_t BufferBegin[] = {10, 0x80, 11, 0x01, 9, 0x19};
        EXPECT_EQ(sizeof BufferBegin, Wire.writeMismatchIndex(BufferBegin, sizeof BufferBegin)) << "writes for begin ok";
        // we are at the default address so the sensor should report it's on
        Wire.setEndTransmissionTogglePeriod(1);
        EXPECT_TRUE(sensor.isOn()) << "Sensor on";
        // reset buffer
        Wire.begin();
        SensorData sample{};
        sensor.read(sample);
        // the mock returns values from 0 increasing by 1 for every read
        EXPECT_EQ(0x0100, sample.x) << "X ok";
        EXPECT_EQ(0x0302, sample.y) << "Y ok";
        EXPECT_EQ(0x0504, sample.z) << "Z ok";

        // we configure another address so it reports off
        Wire.setEndTransmissionTogglePeriod(1);
        EXPECT_TRUE(sensor.isOn()) << "Sensor is on";
        EXPECT_FALSE(sensor.isOn()) << "Sensor is off";
        Wire.setEndTransmissionTogglePeriod(1);

        sensor.configureOverSampling(QmcSampling128);
        sensor.configureRate(QmcRate200Hz);
        sensor.configureRange(QmcRange2G);

        // ensure all test variables are reset
        Wire.begin();
        sensor.begin();
        constexpr uint8_t BufferReconfigure[] = {10, 0x80, 11, 0x01, 9, 0x8d};
        EXPECT_EQ(sizeof BufferReconfigure, Wire.writeMismatchIndex(BufferReconfigure, sizeof BufferReconfigure)) << "Writes for reconfigure ok";

    }
}
