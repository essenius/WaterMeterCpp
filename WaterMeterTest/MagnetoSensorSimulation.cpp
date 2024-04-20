// Copyright 2022-2024 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.`

#include "MagnetoSensorSimulation.h"

namespace WaterMeterCppTest {
    MagnetoSensorSimulation::MagnetoSensorSimulation(const char* fileName) : MagnetoSensor(0, nullptr) {
        _fileName = fileName;
        _measurements.open(fileName);
        _measurements.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (!_measurements.is_open()) {
                       throw std::runtime_error("Cannot open file " + std::string(fileName));
        }
        _index = 0;
        _doneReading = false;
    }

    bool MagnetoSensorSimulation::read(SensorData& sample) {
        if (!(_measurements >> sample.x)) {
            _doneReading = true;
            return false;
        }
        _measurements >> sample.y;
        _index++;
        return true;
    }
}
