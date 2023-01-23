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
#include <Wire.h>

struct SensorData {
    short x;
    short y;
    short z;
    int duration;

    void reset() {
        x = 0;
        y = 0;
        z = 0;
        duration = 0;
    }

    bool operator==(const SensorData& other) const {
        return this->x == other.x && this->y == other.y && this->z == other.z;
    }
};

// not using enum classes as we prefer weak typing to make the code more readable

class MagnetoSensor {
public:
    virtual ~MagnetoSensor() = default;
    explicit MagnetoSensor(byte address, TwoWire* wire);
    MagnetoSensor(const MagnetoSensor&) = default;
    MagnetoSensor(MagnetoSensor&&) = default;
    MagnetoSensor& operator=(const MagnetoSensor&) = default;
    MagnetoSensor& operator=(MagnetoSensor&&) = default;

    // Configure the sensor
    virtual bool begin();

    // Configure the sensor according to the configuration parameters (called in begin())
    virtual bool configure() const = 0;

    // configure the wire address if not default (0x0D). Call before begin()
    void configureAddress(byte address);

    virtual double getGain() const = 0;

    virtual int getNoiseRange() const = 0;

    // returns whether the sensor is active
    virtual bool isOn() const;

    virtual bool isReal() const {
        return true;
    }

    // read a sample from the sensor
    virtual bool read(SensorData* sample) const = 0;

    // soft reset the sensor
    virtual void softReset() const = 0;

protected:
    byte _address;
    TwoWire* _wire;
    void setRegister(byte sensorRegister, byte value) const;
};

#endif
