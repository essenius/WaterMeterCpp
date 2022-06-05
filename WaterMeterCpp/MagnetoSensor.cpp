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

// Disabling a few ReSharper issues for code clarity. The issues are related to:
// * conversions that might in theory go wrong, but the sensor won't return too high values.
// * using enum values to do bitwise manipulations

#include "MagnetoSensor.h"
#include "MagnetoSensorQmc.h"
#include "Wire.h"

MagnetoSensor::MagnetoSensor(const byte address, TwoWire* wire): 
    _address(address),
    _wire(wire) {
}

bool MagnetoSensor::begin() {
    softReset();
    return true;
}

void MagnetoSensor::configureAddress(const byte address) {
    _address = address;
}

bool MagnetoSensor::isOn() const {
    _wire->beginTransmission(_address);
    return _wire->endTransmission() == 0;
}

void MagnetoSensor::setRegister(const byte sensorRegister, const byte value) const {
    _wire->beginTransmission(_address);
    _wire->write(sensorRegister);
    _wire->write(value);
    _wire->endTransmission();
}
