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

// Driver for the HMC5883L sensor.

// need the underscores as decimal point is not allowed either.
// ReSharper disable CppInconsistentNaming

#ifndef HEADER_MAGNETOSENSORHMC
#define HEADER_MAGNETOSENSORHMC

#include "MagnetoSensor.h"

enum HmcGain : byte {
    HmcGain0_88 = 0,
    HmcGain1_3 = 0b00100000,
    HmcGain1_9 = 0b01000000,
    HmcGain2_5 = 0b01100000,
    HmcGain4_0 = 0b10000000,
    HmcGain4_7 = 0b10100000,
    HmcGain5_6 = 0b11000000,
    HmcGain8_1 = 0b11100000
};

enum HmcRate : byte {
    HmcRate0_75 = 0,
    HmcRate1_5 = 0b00000100,
    HmcRate3_0 = 0b00001000,
    HmcRate7_5 = 0b00001100,
    HmcRate15 = 0b00010000,
    HmcRate30 = 0b00010100,
    HmcRate75 = 0b00011000
};

enum HmcOverSampling : byte {
    HmcSampling1 = 0,
    HmcSampling2 = 0b00100000,
    HmcSampling4 = 0b01000000,
    HmcSampling8 = 0b01100000
};

enum HmcRegister : byte {
    HmcControlA = 0,
    HmcControlB = 1,
    HmcMode = 2,
    HmcData = 3,
    HmcStatus = 9
};

enum HmcBias : byte {
    HmcNone = 0,
    HmcPositive = 1,
    HmcNegative = 2
};

enum HmcMode : byte {
    HmcContinuous = 0,
    HmcSingle = 1,
    HmcIdle1 = 2,
    HmcIdle2 = 3
};

class MagnetoSensorHmc final : public MagnetoSensor {
public:
    MagnetoSensorHmc();
    bool configure() const override;
    void configureGain(HmcGain gain);
    void configureOverSampling(HmcOverSampling overSampling);
    void configureRate(HmcRate rate);
    float getGain() const override;
    //float getRange() const override;
    int getNoiseRange() const override;
    static float getGain(HmcGain gain);
    void read(SensorData* sample) const override;
    void softReset() const override;
    static bool testInRange(const SensorData* sample);
    bool test() const;

    static constexpr byte DEFAULT_ADDRESS = 0x1E;

private:
    void configure(HmcGain gain, HmcBias bias) const;
    void getTestMeasurement(SensorData* reading) const;
    void startMeasurement() const;
    // 4.7 is not likely to get an overflow, and reasonably accurate
    HmcGain _gain = HmcGain4_7;
    // not really important as we use single measurements
    HmcRate _rate = HmcRate75;
    // highest possible, to reduce noise
    HmcOverSampling _overSampling = HmcSampling8;
};
#endif
