// Copyright 2022-2023 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

// ReSharper disable CppClangTidyBugproneNarrowingConversions
// ReSharper disable CppClangTidyClangDiagnosticImplicitIntConversion
// ReSharper disable CppClangTidyClangDiagnosticEnumEnumConversion
// ReSharper disable CppClangTidyBugproneSuspiciousEnumUsage
// ReSharper disable CppClangTidyBugproneBranchClone -- false positive

#include "MagnetoSensorHmc.h"
#include "Wire.h"

MagnetoSensorHmc::MagnetoSensorHmc(TwoWire* wire) : MagnetoSensor(DEFAULT_ADDRESS, wire) {}

bool MagnetoSensorHmc::configure() const {
    return test();
}

void MagnetoSensorHmc::configure(const HmcGain gain, const HmcBias bias) const {
    setRegister(HmcControlA, _overSampling | _rate | bias);
    setRegister(HmcControlB, gain);
}

void MagnetoSensorHmc::configureGain(const HmcGain gain) {
    _gain = gain;
}

void MagnetoSensorHmc::configureOverSampling(const HmcOverSampling overSampling) {
    _overSampling = overSampling;
}

void MagnetoSensorHmc::configureRate(const HmcRate rate) {
    _rate = rate;
}

double MagnetoSensorHmc::getGain() const {
    return getGain(_gain);
}

int MagnetoSensorHmc::getNoiseRange() const {
    switch (_gain) {
    case HmcGain0_88: return 8;
    case HmcGain1_3: return 5;
    case HmcGain1_9: return 5;
    case HmcGain2_5: return 4;
    case HmcGain4_0: return 4;
    case HmcGain4_7: return 3; // was 4
    case HmcGain5_6:
    default: return 2;
    }
}

double MagnetoSensorHmc::getGain(const HmcGain gain) {
    switch (gain) {
    case HmcGain0_88: return 1370.0;
    case HmcGain1_3: return 1090.0;
    case HmcGain1_9: return 820.0;
    case HmcGain2_5: return 660.0;
    case HmcGain4_0: return 440.0;
    case HmcGain4_7: return 390.0;
    case HmcGain5_6: return 330.0;
    default: return 230.0;
    }

}

void MagnetoSensorHmc::getTestMeasurement(SensorData* reading) const {
    startMeasurement();
    delay(5);
    read(reading);
}

bool MagnetoSensorHmc::read(SensorData* sample) const {
    startMeasurement();

    _wire->beginTransmission(_address);
    _wire->write(HmcData);
    _wire->endTransmission();

    //Read data from each axis, 2 registers per axis
    // order: x MSB, x LSB, z MSB, z LSB, y MSB, y LSB
    constexpr byte BYTES_TO_READ = 6;
    _wire->requestFrom(_address, BYTES_TO_READ);
    while (_wire->available() < BYTES_TO_READ) {}
    sample->x = _wire->read() << 8;
    sample->x |= _wire->read();
    sample->z = _wire->read() << 8;
    sample->z |= _wire->read();
    sample->y = _wire->read() << 8;
    sample->y |= _wire->read();
    // harmonize saturation values across sensors
    if (sample->x == SATURATED) sample->x = SHRT_MIN;
    if (sample->y == SATURATED) sample->y = SHRT_MIN;
    if (sample->z == SATURATED) sample->z = SHRT_MIN;
    return true;
}

void MagnetoSensorHmc::softReset() const {
    configure(_gain, HmcNone);
    SensorData sample{};
    getTestMeasurement(&sample);
}

void MagnetoSensorHmc::startMeasurement() const {
    setRegister(HmcMode, HmcSingle);
}

bool MagnetoSensorHmc::testInRange(const SensorData* sample) {
    constexpr short LOW_THRESHOLD = 243;
    constexpr short HIGH_THRESHOLD = 575;

    return
        sample->x >= LOW_THRESHOLD && sample->x <= HIGH_THRESHOLD &&
        sample->y >= LOW_THRESHOLD && sample->y <= HIGH_THRESHOLD &&
        sample->z >= LOW_THRESHOLD && sample->z <= HIGH_THRESHOLD;
}

bool MagnetoSensorHmc::test() const {
    SensorData sample{};
    configure(HmcGain4_7, HmcPositive);

    // read old value (still with old settings) 
    getTestMeasurement(&sample);

    // now do the test
    getTestMeasurement(&sample);

    const bool passed = testInRange(&sample);

    // end self test mode
    configure(_gain, HmcNone);

    getTestMeasurement(&sample);

    return passed;
}
