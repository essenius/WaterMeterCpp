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
#include "../WaterMeterCpp/MagnetoSensorNull.h"
#include "../WaterMeterCpp/MagnetoSensorReader.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {

    TEST_CLASS(MagnetoSensorNullTest) {

public:
    TEST_METHOD(magnetoSensorReaderNoSensorTest1) {
        EventServer eventServer;
        MagnetoSensorNull nullSensor;
        Assert::IsFalse(nullSensor.configure(), L"Configure returns false");
        Assert::AreEqual(0.0f, nullSensor.getGain(), L"Gain is 0");
        Assert::AreEqual(0, nullSensor.getNoiseRange(), L"Noise range is 0");
        SensorData sample{ 1,1,1,1 };
        Assert::IsFalse(nullSensor.isReal(), L"nullSensor is not a real sensor");
        Assert::IsFalse(nullSensor.read(&sample), L"Read returns false");
        Assert::AreEqual<short>(0, sample.y, L"y was reset");
        // validate that it doesn't break
        nullSensor.softReset();
    }
    };
}