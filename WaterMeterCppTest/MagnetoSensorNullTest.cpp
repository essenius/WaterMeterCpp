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
#include "../WaterMeterCpp/MagnetoSensorNull.h"
#include "../WaterMeterCpp/MagnetoSensorReader.h"

namespace WaterMeterCppTest {

    TEST(MagnetoSensorNullTest, magnetoSensorReaderNoSensorTest1) {
        EventServer eventServer;
        MagnetoSensorNull nullSensor;
        EXPECT_TRUE(nullSensor.handlePowerOn()) << "handlePowerOn returns true";
        EXPECT_EQ(0.0, nullSensor.getGain()) << "Gain is 0";
        EXPECT_EQ(0, nullSensor.getNoiseRange()) << "Noise range is 0";
        SensorData sample{1, 1, 0};
        EXPECT_FALSE(nullSensor.isReal()) << "nullSensor is not a real sensor";
        EXPECT_FALSE(nullSensor.read(sample)) << "Read returns false";
        EXPECT_EQ(0, sample.y) << "y was reset";
        // validate that it doesn't break
        nullSensor.softReset();
    }
}
