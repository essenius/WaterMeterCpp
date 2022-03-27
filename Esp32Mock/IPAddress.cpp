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
// ReSharper disable CppParameterMayBeConst

#include "IPAddress.h"
#include "../WaterMeterCpp/SafeCString.h"

IPAddress::IPAddress(uint8_t oct1, uint8_t oct2, uint8_t oct3, uint8_t oct4) {
    _address.bytes[0] = oct1;
    _address.bytes[1] = oct2;
    _address.bytes[2] = oct3;
    _address.bytes[3] = oct4;
}

IPAddress::IPAddress(const uint8_t* address) {
    memcpy(_address.bytes, address, sizeof _address.bytes);
}

IPAddress& IPAddress::operator=(uint32_t address) {
    _address.dword = address;
    return *this;
}

IPAddress& IPAddress::operator=(const uint8_t* address) {
    memcpy(_address.bytes, address, sizeof _address.bytes);
    return *this;
}

String IPAddress::toString() const {
    char buffer[16];
    safeSprintf(buffer, "%u.%u.%u.%u", _address.bytes[0], _address.bytes[1], _address.bytes[2], _address.bytes[3]);
    return {buffer};
}
