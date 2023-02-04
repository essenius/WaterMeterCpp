// Copyright 2023 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#include "MagnetoSensorMock.h"

void MagnetoSensorMock::setPowerOnFailures(int failures) {
    _powerFailuresLeft = failures;
}

bool MagnetoSensorMock::read(SensorData& sample) const {
    if (_isFlat) {
        sample.reset();
        return true;
    }
    sample.x = 10 - sample.x;
    sample.y = 5 - sample.y;
    return true;
}

void MagnetoSensorMock::setBeginFailures(int failures) {
    _beginFailuresLeft = failures;
}

bool MagnetoSensorMock::handlePowerOn() {
    if (_powerFailuresLeft == 0) return true;
    _powerFailuresLeft--;
    return false;
}

bool MagnetoSensorMock::begin() {
    if (_beginFailuresLeft == 0) return true;
    _beginFailuresLeft--;
    return false;
}
