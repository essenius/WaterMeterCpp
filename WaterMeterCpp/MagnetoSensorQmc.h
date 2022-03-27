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

struct SensorData {
    short x;
    short y;
    short z;
    int duration;
};

// not using enum classes as we prefer weak typing to make the code more readable

enum SensorRange: byte {
    Range2G = 0b00000000,
    // divide by 120 for microTesla
    Range8G = 0b00010000,
    // divide by 30 
};

enum SensorRate: byte {
    Rate10Hz = 0b00000000,
    Rate50Hz = 0b00000100,
    Rate100Hz = 0b00001000,
    Rate200Hz = 0b00001100
};

enum SensorOverSampling: byte {
    Sampling512 = 0b00000000,
    Sampling256 = 0b01000000,
    Sampling128 = 0b10000000,
    Sampling64 = 0b11000000
};

enum SensorRegister: byte {
    Data = 0x00,
    Status = 0x06,
    Control1 = 0x09,
    Control2 = 0x0a,
    SetReset = 0x0b
};

enum SensorMode: byte {
    Standby = 0,
    Continuous = 1
};

// QMC5883L sensor driver returning the raw readings.

class MagnetoSensor {
public:
    // Start Wire and configure the sensor
    void begin() const;

    // Configure the sensor according to the configuration parameters (called in begin())
    void configure() const;

    // configure the wire address if not default (0x0D). Call before begin()
    void configureAddress(byte address);

    // configure oversampling if not default (Sampling512). Do before begin()
    void configureOverSampling(SensorOverSampling overSampling);

    // Configure the GPIO port used for the sensor power if not default (15). 
    void configurePowerPort(uint8_t port);

    // configure the range if not default (Range8G). Call before begin()
    void configureRange(SensorRange range);

    // configure the rate if not default (Rate100Hz). Call before begin()
    // Note: lower rates won't work with the water meter as the code expects 100 Hz.
    void configureRate(SensorRate rate);

    // returns whether the sensor has data available
    bool dataReady() const;

    // power cycle the sensor
    void hardReset() const;

    // returns whether the sensor is active
    bool isOn() const;

    // read a sample from the sensor
    void read(SensorData* sample) const;

    // soft reset the sensor
    void softReset() const;

    static constexpr byte DEFAULT_ADDRESS = 0x0D;
    static constexpr byte DEFAULT_POWER_PORT = 15;

private:
    uint8_t _powerPort = DEFAULT_POWER_PORT;
    byte _address = DEFAULT_ADDRESS;
    SensorOverSampling _overSampling = Sampling512;
    SensorRange _range = Range8G;
    SensorRate _rate = Rate100Hz;
    byte getRegister(SensorRegister sensorRegister) const;
    void setRegister(SensorRegister sensorRegister, byte value) const;
};

#endif
