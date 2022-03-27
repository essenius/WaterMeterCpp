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

// Mock implementation for unit testing (not targeting the ESP32)

// Disabling warnings caused by mimicking existing interface
// ReSharper disable CppInconsistentNaming

#ifndef WIRE_HEADER
#define WIRE_HEADER

#include "Wire.h"
#include "ESP.h"

class TwoWire {
public:
    int available();
    void begin();
    void beginTransmission(uint8_t address);
    int read();
    size_t write(uint8_t value);
    uint8_t endTransmission();
    uint8_t requestFrom(uint8_t address, uint8_t size);

    // testing - get the configured address
    uint8_t getAddress();

    // testing - set the toggle period for the EndTransmission result to test e.g. sensor.isOn
    void setEndTransmissionTogglePeriod(int period);

    // testing - force reads to return the same value
    void setFlatline(bool flatline);

    // testing - return -1 if length wrong, index of wrong item if content wrong, or length if all OK.
    short writeMismatchIndex(const uint8_t* expected, short length) const;

private:
    static constexpr short WRITE_BUFFER_SIZE = 1024;
    uint8_t _address = 0;
    int _available = 0;
    uint8_t _nextResult = 0;
    uint8_t _written[WRITE_BUFFER_SIZE]{};
    short _writeIndex = 0;
    bool _flatline = false;
    int _endTransmissionCounter = 10;
    uint8_t _endTransmissionValue = 0;
    int _endTransmissionTogglePeriod = 10;
};

extern TwoWire Wire;
#endif
