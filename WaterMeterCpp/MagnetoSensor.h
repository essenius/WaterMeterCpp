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

#ifndef HEADER_MAGNETOSENSOR
#define HEADER_MAGNETOSENSOR

// OK with the unscoped types
#pragma warning (disable:26812)

#include <ESP.h>

struct SensorData {
    short x;
    short y;
    short z;
    int duration;
};

// not using enum classes as we prefer weak typing to make the code more readable

class MagnetoSensor {
public:
    virtual ~MagnetoSensor() = default;
    explicit MagnetoSensor(byte address);

    // Start Wire and configure the sensor
    void begin() const;

    // Configure the sensor according to the configuration parameters (called in begin())
    virtual void configure() const = 0;

    // configure the wire address if not default (0x0D). Call before begin()
    void configureAddress(byte address);

    // Configure the GPIO port used for the sensor power if not default (15). 
    void configurePowerPort(uint8_t port);

    // returns whether the sensor has data available
    virtual bool dataReady() const = 0;

    // power cycle the sensor
    void hardReset() const;

    // returns whether the sensor is active
    bool isOn() const;

    // read a sample from the sensor
    virtual void read(SensorData* sample) const = 0;

    // soft reset the sensor
    virtual void softReset() const = 0;

    static constexpr byte DEFAULT_POWER_PORT = 15;

protected:
    uint8_t _powerPort = DEFAULT_POWER_PORT;
    byte _address;
    byte getRegister(byte sensorRegister) const;
    void setRegister(byte sensorRegister, byte value) const;
};

#endif
