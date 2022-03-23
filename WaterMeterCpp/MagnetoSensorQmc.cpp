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

#include "MagnetoSensorQmc.h"
#include "Wire.h"

constexpr int SOFT_RESET = 0x80;
constexpr int DATA_READY = 0x01;

void MagnetoSensor::begin() {
    Wire.begin();
    reset();
    configure();
}

void MagnetoSensor::begin(const SensorRange range) {
    _range = range;
    begin();
}

void MagnetoSensor::configure() {
    setRegister(SetReset, 0x01);
    setRegister(Control1, Continuous | _rate | _range | _overSampling);
}

bool MagnetoSensor::dataReady() {
    return (getRegister(Status) & DATA_READY) != 0;
}

byte MagnetoSensor::getRegister(const SensorRegister sensorRegister) {
    constexpr byte BYTES_TO_READ = 1;
    Wire.beginTransmission(ADDRESS);
    Wire.write(sensorRegister);
    Wire.endTransmission();
    Wire.requestFrom(ADDRESS, BYTES_TO_READ);
    const byte value = static_cast<byte>(Wire.read());
    Wire.endTransmission();
    return value;
}

bool MagnetoSensor::isOn() {
    Wire.beginTransmission(ADDRESS);
    return Wire.endTransmission() == 0;
}

void MagnetoSensor::read(SensorData* sample) {
    Wire.beginTransmission(ADDRESS);
    Wire.write(Data); 
    Wire.endTransmission();
  
    // Read data from each axis, 2 registers per axis
    // order: x LSB, x MSB, y LSB, y MSB, z LSB, z MSB
    constexpr byte BYTES_TO_READ = 6;
    Wire.requestFrom(ADDRESS, BYTES_TO_READ);
    while (Wire.available() < BYTES_TO_READ) {}
    sample->x = Wire.read() | Wire.read()<<8; 
    sample->y = Wire.read() | Wire.read()<<8; 
    sample->z = Wire.read() | Wire.read()<<8;
}

void MagnetoSensor::reset() {
    setRegister(Control2, SOFT_RESET);
}

void MagnetoSensor::setRange(const SensorRange range) {
    _range = range;
    configure();
}

void MagnetoSensor::setRegister(const SensorRegister sensorRegister, const byte value) {
    Wire.beginTransmission(ADDRESS); 
    Wire.write(sensorRegister); 
    Wire.write(value); 
    Wire.endTransmission();  
}
