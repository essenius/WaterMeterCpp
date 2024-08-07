// Copyright 2022-2024 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

// If we don't detect a sensor, we use this null sensor instead. That makes the code a bit cleaner.

#ifndef HEADER_MAGNETO_SENSOR_TEST
#define HEADER_MAGNETO_SENSOR_TEST

#include <fstream>
#include <MagnetoSensor.h>

namespace WaterMeterCppTest {
    using MagnetoSensors::MagnetoSensor;
    using MagnetoSensors::SensorData;

    class MagnetoSensorSimulation final : public MagnetoSensor {
    public:
        MagnetoSensorSimulation() : MagnetoSensorSimulation("testData\\rawSensorData.txt") {}
        explicit MagnetoSensorSimulation(const char* fileName);


        bool begin() override {
            _index = 0;
            return true;
        }

        double getGain() const override {
            return 390;
        }

        int getNoiseRange() const override {
            return 3;
        }

        bool isOn() override {
            return true;
        }

        bool isReal() const override {
            return false;
        }

        bool read(SensorData& sample) override;

        bool done() const {
            return _doneReading;
        }

        void softReset() override { _index = 0; }

        void waitForPowerOff() override {}

    private:
        const char* _fileName;
        std::ifstream _measurements;
        bool _doneReading = false;
        int _index = 0;
    };
}
#endif