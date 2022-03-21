// Copyright 2021-2022 Rik Essenius
// 
//   Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//   except in compliance with the License. You may obtain a copy of the License at
// 
//       http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software distributed under the License
//    is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and limitations under the License.

// Mock implementation for unit testing (not targeting the ESP32)

// ReSharper disable CppInconsistentNaming

#ifndef ESP32

#ifndef WIREMOCK_HEADER
#define WIREMOCK_HEADER

#include "Wire.h"
#include "ESP.h"

class TwoWire {
public:
    int available();
    void begin();
    void beginTransmission(uint8_t address);
    int read();
    size_t write(uint8_t reg);
    uint8_t endTransmission();
    uint8_t requestFrom(uint8_t address, uint8_t size);
};

extern TwoWire Wire;
#endif
#endif