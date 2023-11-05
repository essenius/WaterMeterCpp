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
#include "Wire.h"
#include "../WaterMeterCpp/MagnetoSensorHmc.h"

namespace WaterMeterCppTest {

    TEST(MagnetoSensorHmcTest, magnetoSensorHmcCustomAddressTest) {
        MagnetoSensorHmc sensor;
        constexpr uint8_t Address = 0x23;
        sensor.configureAddress(Address);
        Wire.begin();
        sensor.begin();
        EXPECT_EQ(Address, Wire.getAddress()) << "Custom Address OK";
    }

    TEST(MagnetoSensorHmcTest, magnetoSensorHmcGetGainTest) {
        EXPECT_EQ(1370.0, MagnetoSensorHmc::getGain(HmcRange0_88)) << "0.88G gain ok";
        EXPECT_EQ(1090.0, MagnetoSensorHmc::getGain(HmcRange1_3)) << "1.3G gain ok";
        EXPECT_EQ(820.0, MagnetoSensorHmc::getGain(HmcRange1_9)) << "1.9G gain ok";
        EXPECT_EQ(660.0, MagnetoSensorHmc::getGain(HmcRange2_5)) << "2.5G gain ok";
        EXPECT_EQ(440.0, MagnetoSensorHmc::getGain(HmcRange4_0)) << "4.0G gain ok";
        EXPECT_EQ(390.0, MagnetoSensorHmc::getGain(HmcRange4_7)) << "4.7G gain ok";
        EXPECT_EQ(330.0, MagnetoSensorHmc::getGain(HmcRange5_6)) << "5.6G gain ok";
        EXPECT_EQ(230.0, MagnetoSensorHmc::getGain(HmcRange8_1)) << "8.1G gain ok";
        MagnetoSensorHmc sensor;
        sensor.configureRange(HmcRange2_5);
        EXPECT_EQ(660.f, sensor.getGain()) << "getGain() returns correct value";
    }


    TEST(MagnetoSensorHmcTest, magnetoSensorHmcTestTest) {
        MagnetoSensorHmc sensor;
        Wire.begin();
        EXPECT_FALSE(sensor.handlePowerOn()) << "Sensor Test failed";
    }

    TEST(MagnetoSensorHmcTest, magnetoSensorHmcTestInRangeTest) {
        SensorData sensorData{200, 400, 400};
        EXPECT_FALSE(MagnetoSensorHmc::testInRange(sensorData)) << "Low X";
        sensorData.x = 600;
        EXPECT_FALSE(MagnetoSensorHmc::testInRange(sensorData)) << "High X";
        sensorData.x = 400;
        EXPECT_TRUE(MagnetoSensorHmc::testInRange(sensorData)) << "Within bounds";
        sensorData.y = 200;
        EXPECT_FALSE(MagnetoSensorHmc::testInRange(sensorData)) << "Low Y";
        sensorData.y = 600;
        EXPECT_FALSE(MagnetoSensorHmc::testInRange(sensorData)) << "High Y";
        sensorData.z = 200;
        EXPECT_FALSE(MagnetoSensorHmc::testInRange(sensorData)) << "Low Z and high Y";
        sensorData.y = 400;
        EXPECT_FALSE(MagnetoSensorHmc::testInRange(sensorData)) << "Low Z";
        sensorData.z = 600;
        EXPECT_FALSE(MagnetoSensorHmc::testInRange(sensorData)) << "High Z";
    }

    TEST(MagnetoSensorHmcTest, magnetoSensorHmcScriptTest) {
        MagnetoSensorHmc sensor;
        setRealTime(true);
        Wire.begin();
        sensor.begin();
        EXPECT_EQ(MagnetoSensorHmc::DefaultAddress, Wire.getAddress()) << "Default address OK";
        constexpr uint8_t BufferBegin[] = {0, 0x78, 1, 0xa0, 2, 0x01, 2, 0x01, 3};
        EXPECT_EQ(sizeof BufferBegin, Wire.writeMismatchIndex(BufferBegin, sizeof BufferBegin)) << "writes for begin ok";
        // we are at the default address so the sensor should report it's on
        Wire.setEndTransmissionTogglePeriod(1);
        EXPECT_TRUE(sensor.isOn()) << "Sensor is on";
        // reset buffer
        Wire.begin();
        SensorData sample{};
        sensor.read(sample);
        // the mock returns values from 0 increasing by 1 for every read
        EXPECT_EQ(0x001, sample.x) << "X ok";
        EXPECT_EQ(0x0405, sample.y) << "Y ok";
        EXPECT_EQ(0x0203, sample.z) << "Z ok";

        // force a lower value than the saturation. This will be 0xeeee, which is -4370, i.e. less than -4096
        constexpr int Saturated = 0xee;
        Wire.setFlatline(true, Saturated);
        sensor.read(sample);
        EXPECT_EQ(SHRT_MIN, sample.x) << "X saturated";
        EXPECT_EQ(SHRT_MIN, sample.y) << "Y saturated";
        EXPECT_EQ(SHRT_MIN, sample.z) << "Z saturated";
        Wire.setFlatline(false, 0);

        Wire.setEndTransmissionTogglePeriod(1);
        EXPECT_TRUE(sensor.isOn()) << "Sensor is on";
        EXPECT_FALSE(sensor.isOn()) << "Sensor is off";
        Wire.setEndTransmissionTogglePeriod(1);

        sensor.configureOverSampling(HmcSampling4); // 0b0100 0000
        sensor.configureRate(HmcRate30); // 0b0001 0100
        sensor.configureRange(HmcRange5_6); // 0b1100 0000

        // ensure all test variables are reset
        Wire.begin();
        sensor.begin();
        constexpr uint8_t BufferReconfigure[] = {0, 0x54, 1, 0xc0, 2, 0x01, 2, 0x01, 3};
        EXPECT_EQ(sizeof BufferReconfigure, Wire.writeMismatchIndex(BufferReconfigure, sizeof BufferReconfigure)) << "writes for reconfigure ok";
    }
}

