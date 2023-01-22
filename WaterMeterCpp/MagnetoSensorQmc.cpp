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

MagnetoSensorQmc::MagnetoSensorQmc(TwoWire* wire): MagnetoSensor(DEFAULT_ADDRESS, wire) {}

bool MagnetoSensorQmc::configure() const {
    setRegister(QmcSetReset, 0x01);
    setRegister(QmcControl1, QmcContinuous | _rate | _range | _overSampling);
    return true;
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

// if we ever need DataReady, use (getRegister(QmcStatus) & 0x01) != 0;

double MagnetoSensorQmc::getGain() const {
    return getGain(_range);
}

double MagnetoSensorQmc::getGain(const QmcRange range) {
    if (range == QmcRange8G) return 3000.0;
    return 12000.0;
}

bool MagnetoSensorQmc::read(SensorData* sample) const {
    _wire->beginTransmission(_address);
    _wire->write(QmcData);
    _wire->endTransmission();

    constexpr byte BYTES_TO_READ = 6;
    constexpr byte BITS_PER_BYTE = 8;
    // Read data from each axis, 2 registers per axis
    // order: x LSB, x MSB, y LSB, y MSB, z LSB, z MSB
    _wire->requestFrom(_address, BYTES_TO_READ);
    while (_wire->available() < BYTES_TO_READ) {}
    sample->x = _wire->read() | _wire->read() << BITS_PER_BYTE;
    sample->y = _wire->read() | _wire->read() << BITS_PER_BYTE;
    sample->z = _wire->read() | _wire->read() << BITS_PER_BYTE;
    // no need to adjust saturation values as it already uses SHRT_MIN and SHRT_MAX
    return true;
}

void MagnetoSensorQmc::softReset() const {
    setRegister(QmcControl2, SOFT_RESET);
    static_cast<void>(configure());
}

int MagnetoSensorQmc::getNoiseRange() const {
    // only checked on 8 Gauss
    return 60;
}
