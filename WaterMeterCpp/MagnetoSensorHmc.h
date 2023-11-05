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

// Driver for the HMC5883L sensor.
// Datasheet: see https://cdn-shop.adafruit.com/datasheets/HMC5883L_3-Axis_Digital_Compass_IC.pdf

// need the underscores as decimal point is not allowed either.
// ReSharper disable CppInconsistentNaming

#ifndef HEADER_MAGNETOSENSORHMC
#define HEADER_MAGNETOSENSORHMC

#include "MagnetoSensor.h"


// this sensor has several ranges that you can configure.

enum HmcRange : byte {
    HmcRange0_88 = 0,
    HmcRange1_3 = 0b00100000,
    HmcRange1_9 = 0b01000000,
    HmcRange2_5 = 0b01100000,
    HmcRange4_0 = 0b10000000,
    HmcRange4_7 = 0b10100000,
    HmcRange5_6 = 0b11000000,
    HmcRange8_1 = 0b11100000
};

// The sensor can be configured to return samples at different rates

enum HmcRate : byte {
    HmcRate0_75 = 0,
    HmcRate1_5 = 0b00000100,
    HmcRate3_0 = 0b00001000,
    HmcRate7_5 = 0b00001100,
    HmcRate15 = 0b00010000,
    HmcRate30 = 0b00010100,
    HmcRate75 = 0b00011000
};

// Each sample can be an average of a number of "raw" samples

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
    explicit MagnetoSensorHmc(TwoWire* wire = &Wire);
    void configureRange(HmcRange range);
    void configureOverSampling(HmcOverSampling overSampling);
    void configureRate(HmcRate rate);
    double getGain() const override;
    int getNoiseRange() const override;
    static double getGain(HmcRange range);
    bool read(SensorData& sample) const override;
    void softReset() const override;
    static bool testInRange(const SensorData& sample);
    bool test() const;
    static constexpr byte DefaultAddress = 0x1E;
    bool handlePowerOn() override;
private:
    static constexpr int16_t Saturated = -4096;
    void configure(HmcRange range, HmcBias bias) const;
    void getTestMeasurement(SensorData& reading) const;
    void startMeasurement() const;

    // 4.7 is not likely to get an overflow, and reasonably accurate
    HmcRange _range = HmcRange4_7;
    // not really important as we use single measurements
    HmcRate _rate = HmcRate75;
    // highest possible, to reduce noise
    HmcOverSampling _overSampling = HmcSampling8;
};
#endif
