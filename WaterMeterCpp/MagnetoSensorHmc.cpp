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

namespace WaterMeter {
    MagnetoSensorHmc::MagnetoSensorHmc(TwoWire* wire) : MagnetoSensor(DefaultAddress, wire) {}

    void MagnetoSensorHmc::configure(const HmcRange range, const HmcBias bias) const {
        setRegister(HmcControlA, _overSampling | _rate | bias);
        setRegister(HmcControlB, range);
    }

    void MagnetoSensorHmc::configureRange(const HmcRange range) {
        _range = range;
    }

    void MagnetoSensorHmc::configureOverSampling(const HmcOverSampling overSampling) {
        _overSampling = overSampling;
    }

    void MagnetoSensorHmc::configureRate(const HmcRate rate) {
        _rate = rate;
    }

    double MagnetoSensorHmc::getGain() const {
        return getGain(_range);
    }

    int MagnetoSensorHmc::getNoiseRange() const {
        switch (_range) {
        case HmcRange0_88: return 8;
        case HmcRange1_3: return 5;
        case HmcRange1_9: return 5;
        case HmcRange2_5: return 4;
        case HmcRange4_0: return 4;
        case HmcRange4_7: return 3; // was 4
        case HmcRange5_6:
        default: return 2;
        }
    }

    double MagnetoSensorHmc::getGain(const HmcRange range) {
        switch (range) {
        case HmcRange0_88: return 1370.0;
        case HmcRange1_3: return 1090.0;
        case HmcRange1_9: return 820.0;
        case HmcRange2_5: return 660.0;
        case HmcRange4_0: return 440.0;
        case HmcRange4_7: return 390.0;
        case HmcRange5_6: return 330.0;
        default: return 230.0;
        }

    }

    void MagnetoSensorHmc::getTestMeasurement(SensorData& reading) {
        startMeasurement();
        delay(5);
        read(reading);
    }

    bool MagnetoSensorHmc::read(SensorData& sample) {
        startMeasurement();

        _wire->beginTransmission(_address);
        _wire->write(HmcData);
        _wire->endTransmission();

        //Read data from each axis, 2 registers per axis
        // order: x MSB, x LSB, z MSB, z LSB, y MSB, y LSB
        constexpr size_t BytesToRead = 6;
        _wire->requestFrom(_address, BytesToRead, StopAfterSend);
        const auto timestamp = micros();
        while (_wire->available() < BytesToRead) {
            if (micros() - timestamp > 10) return false;
        }

        sample.x = _wire->read() << 8;
        sample.x |= _wire->read();
        sample.z = _wire->read() << 8;
        sample.z |= _wire->read();
        sample.y = _wire->read() << 8;
        sample.y |= _wire->read();
        // harmonize saturation values across sensors
        if (sample.x <= Saturated) sample.x = SHRT_MIN;
        if (sample.y <= Saturated) sample.y = SHRT_MIN;
        if (sample.z <= Saturated) sample.z = SHRT_MIN;
        return true;
    }

    void MagnetoSensorHmc::softReset() {
        configure(_range, HmcNone);
        SensorData sample{};
        getTestMeasurement(sample);
    }

    void MagnetoSensorHmc::startMeasurement() const {
        setRegister(HmcMode, HmcSingle);
    }

    bool MagnetoSensorHmc::testInRange(const SensorData& sample) {
        constexpr short LowThreshold = 243;
        constexpr short HighThreshold = 575;

        return
            sample.x >= LowThreshold && sample.x <= HighThreshold &&
            sample.y >= LowThreshold && sample.y <= HighThreshold &&
            sample.z >= LowThreshold && sample.z <= HighThreshold;
    }

    bool MagnetoSensorHmc::test() {
        SensorData sample{};

        configure(HmcRange4_7, HmcPositive);

        // read old value (still with old settings) 
        getTestMeasurement(sample);

        // now do the test
        getTestMeasurement(sample);
        const bool passed = testInRange(sample);

        // end self test mode
        configure(_range, HmcNone);
        // skip the final measurement with the old gain
        getTestMeasurement(sample);

        return passed;
    }

    bool MagnetoSensorHmc::handlePowerOn() {
        return test();
    }
}