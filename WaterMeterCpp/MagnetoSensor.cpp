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


MagnetoSensor::MagnetoSensor(const byte address): _address(address) {
}

void MagnetoSensor::begin() const {
    softReset();
}

void MagnetoSensor::configureAddress(const byte address) {
    _address = address;
}

void MagnetoSensor::configurePowerPort(const uint8_t port) {
    _powerPort = port;
}

byte MagnetoSensor::getRegister(const byte sensorRegister) const {
    constexpr byte BYTES_TO_READ = 1;
    Wire.beginTransmission(_address);
    Wire.write(sensorRegister);
    Wire.endTransmission();
    Wire.requestFrom(_address, BYTES_TO_READ);
    const byte value = static_cast<byte>(Wire.read());
    Wire.endTransmission();
    return value;
}

void MagnetoSensor::hardReset() const {
    digitalWrite(_powerPort, LOW);
    while (isOn()) {}
    digitalWrite(_powerPort, HIGH);
    while (!isOn()) {}
    delay(10);
    // since this was drastic, we might need to revive Wire too.
    begin();
}

bool MagnetoSensor::isOn() const {
    Wire.beginTransmission(_address);
    return Wire.endTransmission() == 0;
}

void MagnetoSensor::setRegister(const byte sensorRegister, const byte value) const {
    Wire.beginTransmission(_address);
    Wire.write(sensorRegister);
    Wire.write(value);
    Wire.endTransmission();
}