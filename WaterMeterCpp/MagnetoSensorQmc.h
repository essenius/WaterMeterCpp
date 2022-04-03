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
    Range2G = 0b00000000,
    // divide by 120 for microTesla
    Range8G = 0b00010000,
    // divide by 30 
};

enum QmcRate: byte {
    Rate10Hz = 0b00000000,
    Rate50Hz = 0b00000100,
    Rate100Hz = 0b00001000,
    Rate200Hz = 0b00001100
};

enum QmcOverSampling: byte {
    Sampling512 = 0b00000000,
    Sampling256 = 0b01000000,
    Sampling128 = 0b10000000,
    Sampling64 = 0b11000000
};

enum QmcRegister: byte {
    Data = 0x00,
    Status = 0x06,
    Control1 = 0x09,
    Control2 = 0x0a,
    SetReset = 0x0b
};

enum QmcMode: byte {
    Standby = 0,
    Continuous = 1
};

// QMC5883L sensor driver returning the raw readings.

class MagnetoSensorQmc final : public MagnetoSensor {
public:
    MagnetoSensorQmc();
    // Configure the sensor according to the configuration parameters (called in begin())
    void configure() const override;

    // configure oversampling if not default (Sampling512). Do before begin()
    void configureOverSampling(QmcOverSampling overSampling);

    // configure the range if not default (Range8G). Call before begin()
    void configureRange(QmcRange range);

    // configure the rate if not default (Rate100Hz). Call before begin()
    // Note: lower rates won't work with the water meter as the code expects 100 Hz.
    void configureRate(QmcRate rate);

    // returns whether the sensor has data available
    bool dataReady() const override;

    // read a sample from the sensor
    void read(SensorData* sample) const override;

    // soft reset the sensor
    void softReset() const override;

    static constexpr byte DEFAULT_ADDRESS = 0x0D;

private:
    QmcOverSampling _overSampling = Sampling512;
    QmcRange _range = Range8G;
    QmcRate _rate = Rate100Hz;
};

#endif
