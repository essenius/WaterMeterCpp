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

// Driver for the QMC5883L sensor.
// We don't care a lot about calibration as we're looking for peaks in the signals.
//
// Since the sensor sometimes stops responding, we need a way to hard reset it.
// For that, we simply give it its power from a GPIO port, which we can bring down to reset it.

#ifndef HEADER_MAGNETOSENSORQMC
#define HEADER_MAGNETOSENSORQMC

// OK with the unscoped types
#pragma warning (disable:26812)

#include "ESP.h"
#include "MagnetoSensor.h"

// not using enum classes as we prefer weak typing to make the code more readable

enum QmcRange: byte {
    QmcRange2G = 0b00000000,
    // divide by 120 for microTesla
    QmcRange8G = 0b00010000,
    // divide by 30 
};

enum QmcRate: byte {
    QmcRate10Hz = 0b00000000,
    QmcRate50Hz = 0b00000100,
    QmcRate100Hz = 0b00001000,
    QmcRate200Hz = 0b00001100
};

enum QmcOverSampling: byte {
    QmcSampling512 = 0b00000000,
    QmcSampling256 = 0b01000000,
    QmcSampling128 = 0b10000000,
    QmcSampling64 = 0b11000000
};

enum QmcRegister: byte {
    QmcData = 0x00,
    QmcStatus = 0x06,
    QmcControl1 = 0x09,
    QmcControl2 = 0x0a,
    QmcSetReset = 0x0b
};

enum QmcMode: byte {
    QmcStandby = 0,
    QmcContinuous = 1
};

// QMC5883L sensor driver returning the raw readings.

class MagnetoSensorQmc final : public MagnetoSensor {
public:
    explicit MagnetoSensorQmc(TwoWire* wire = &Wire);
    // Configure the sensor according to the configuration parameters (called in begin())
    bool configure() const override;

    // configure oversampling if not default (QmcSampling512). Do before begin()
    void configureOverSampling(QmcOverSampling overSampling);

    // configure the range if not default (QmcRange8G). Call before begin()
    void configureRange(QmcRange range);

    // configure the rate if not default (QmcRate100Hz). Call before begin()
    // Note: lower rates won't work with the water meter as the code expects 100 Hz.
    void configureRate(QmcRate rate);

    double getGain() const override;

    static double getGain(QmcRange range);

    // read a sample from the sensor
    bool read(SensorData* sample) const override;

    // soft reset the sensor
    void softReset() const override;
    int getNoiseRange() const override;

    static constexpr byte DEFAULT_ADDRESS = 0x0D;

private:
    QmcOverSampling _overSampling = QmcSampling512;
    QmcRange _range = QmcRange8G;
    QmcRate _rate = QmcRate100Hz;
};

#endif
