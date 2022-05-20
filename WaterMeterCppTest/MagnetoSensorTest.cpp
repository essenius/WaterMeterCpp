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

    TEST_CLASS(MagnetoSensorTest) {
    public:
        TEST_METHOD(magnetoSensorDefaultPowerPinTest) {
            const MagnetoSensorQmc sensor;
            digitalWrite(MagnetoSensor::DEFAULT_POWER_PORT, LOW);
            Wire.begin();
            sensor.begin();
            sensor.hardReset();
            Assert::AreEqual<int>(MagnetoSensorQmc::DEFAULT_ADDRESS, Wire.getAddress(), L"Default address OK");
            Assert::AreEqual<int>(HIGH, digitalRead(MagnetoSensor::DEFAULT_POWER_PORT), L"Default Pin was toggled");
        }

        TEST_METHOD(magnetoSensorCustomPowerPinTest) {
            MagnetoSensorQmc sensor;
            constexpr uint8_t PIN = 23;
            sensor.configurePowerPort(PIN);
            digitalWrite(PIN, LOW);
            Wire.begin();
            // make isOn pass first time
            Wire.setEndTransmissionTogglePeriod(1);
            sensor.begin();
            sensor.hardReset();
            Assert::AreEqual<int>(HIGH, digitalRead(PIN), L"Custom pin was toggled");
            Assert::AreEqual<int>(MagnetoSensorQmc::DEFAULT_ADDRESS, Wire.getAddress(), L"Default address OK");
        }
    };
}
