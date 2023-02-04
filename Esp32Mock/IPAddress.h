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

// Mock implementation of the IPAddress class for unit testing (not targeting the ESP32)

// Disabling warnings caused by mimicking existing interface
// ReSharper disable CppParameterMayBeConst
// ReSharper disable CppInconsistentNaming
// ReSharper disable CppNonExplicitConversionOperator

#ifndef HEADER_IPADDRESS
#define HEADER_IPADDRESS

#include <cstdint>
#include "StringArduino.h"

class IPAddress {
public:
    IPAddress() = default;
    IPAddress(uint8_t oct1, uint8_t oct2, uint8_t oct3, uint8_t oct4);
    explicit IPAddress(const uint8_t* address);
    String toString() const;
    IPAddress& operator=(uint32_t address);
    IPAddress& operator=(const uint8_t* address);
    operator uint32_t() const { return _address.dword; }
    uint8_t operator[](int index) const { return _address.bytes[index]; }
    uint8_t& operator[](int index) { return _address.bytes[index]; }

private:
    union {
        uint8_t bytes[4];
        uint32_t dword;
    } _address{};

    uint8_t* raw_address() { return _address.bytes; }
};

const IPAddress INADDR_NONE(0, 0, 0, 0);

#endif
