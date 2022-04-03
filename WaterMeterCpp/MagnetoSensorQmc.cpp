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

// ReSharper disable CppClangTidyBugproneNarrowingConversions
// ReSharper disable CppClangTidyClangDiagnosticImplicitIntConversion
// ReSharper disable CppClangTidyClangDiagnosticEnumEnumConversion
// ReSharper disable CppClangTidyBugproneSuspiciousEnumUsage

#include "MagnetoSensorQmc.h"
#include "Wire.h"

constexpr int SOFT_RESET = 0x80;
constexpr int DATA_READY = 0x01;

MagnetoSensorQmc::MagnetoSensorQmc(): MagnetoSensor(DEFAULT_ADDRESS) {}

void MagnetoSensorQmc::configure() const {
    setRegister(SetReset, 0x01);
    setRegister(Control1, Continuous | _rate | _range | _overSampling);
}

void MagnetoSensorQmc::configureOverSampling(const QmcOverSampling overSampling) {
    _overSampling = overSampling;
}

void MagnetoSensorQmc::configureRange(const QmcRange range) {
    _range = range;
}

void MagnetoSensorQmc::configureRate(const QmcRate rate) {
    _rate = rate;
}

bool MagnetoSensorQmc::dataReady() const {
    return (getRegister(Status) & DATA_READY) != 0;
}

void MagnetoSensorQmc::read(SensorData* sample) const {
    Wire.beginTransmission(_address);
    Wire.write(Data); 
    Wire.endTransmission();
  
    constexpr byte BYTES_TO_READ = 6;
    constexpr byte BITS_PER_BYTE = 8;
    // Read data from each axis, 2 registers per axis
    // order: x LSB, x MSB, y LSB, y MSB, z LSB, z MSB
    Wire.requestFrom(_address, BYTES_TO_READ);
    while (Wire.available() < BYTES_TO_READ) {}
    sample->x = Wire.read() | Wire.read() << BITS_PER_BYTE;
    sample->y = Wire.read() | Wire.read() << BITS_PER_BYTE;
    sample->z = Wire.read() | Wire.read() << BITS_PER_BYTE;
}

void MagnetoSensorQmc::softReset() const {
    setRegister(Control2, SOFT_RESET);
    configure();
}
