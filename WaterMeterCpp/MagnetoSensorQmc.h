// Copyright 2022 Rik Essenius
// 
//   Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//   except in compliance with the License. You may obtain a copy of the License at
// 
//       http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software distributed under the License
//    is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and limitations under the License.

#ifndef HEADER_MAGNETOSENSORQMC
#define HEADER_MAGNETOSENSORQMC

#include "ESP.h"

struct SensorData {
    short x;
    short y;
    short z;
    int duration;
};

// not using enum classes as we prefer weak typing to make the code more readable

enum SensorRange: byte {
    Range2G = 0b00000000, // divide by 120 for microTesla
    Range8G = 0b00010000, // divide by 30 
};

enum SensorRate: byte {
    Rate10Hz  = 0b00000000,
    Rate50Hz  = 0b00000100,
    Rate100Hz = 0b00001000,
    Rate200Hz = 0b00001100
};

enum SensorOverSampling: byte {
    Sampling512 = 0b00000000,
    Sampling256 = 0b01000000,
    Sampling128 = 0b10000000,
    Sampling64  = 0b11000000
};

enum SensorRegister: byte {
    Data     = 0x00,
    Status   = 0x06,
    Control1 = 0x09,
    Control2 = 0x0a,
    SetReset = 0x0b
};

enum SensorMode: byte {
    Standby    = 0,
    Continuous = 1
};

// QMC5883L sensor driver returning the raw readings.

class MagnetoSensor {
public:
    void begin();
    void begin(SensorRange range);
    bool dataReady();
    bool isOn();
    void read(SensorData* sample);
    void reset();
    void setRange(SensorRange range);

private:
    static constexpr byte ADDRESS = 0x0D; // address of QMC5883L

    void configure();
    byte getRegister(SensorRegister sensorRegister);
    void setRegister(SensorRegister sensorRegister, byte value);

    SensorRange _range = SensorRange::Range8G;
    SensorRate _rate = SensorRate::Rate100Hz;
    SensorOverSampling _overSampling = SensorOverSampling::Sampling512;
};

#endif
