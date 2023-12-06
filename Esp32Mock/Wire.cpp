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
// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppMemberFunctionMayBeStatic
// ReSharper disable CppParameterMayBeConst
// ReSharper disable CppParameterNeverUsed

#include <Wire.h>

TwoWire Wire;
TwoWire Wire1;

int TwoWire::available() {
    return ++_available;
}

void TwoWire::begin(int sda, int scl) {
    _nextResult = 0;
    _available = 0;
    _writeIndex = 0;
    _endTransmissionValue = 0;
    _endTransmissionCounter = _endTransmissionTogglePeriod;
    _flatline = false;
}

void TwoWire::beginTransmission(uint8_t address) {
    _address = address;
}

uint8_t TwoWire::endTransmission() {
    if (_endTransmissionTogglePeriod == 0) {
        return _endTransmissionValue;
    }
    if (_endTransmissionCounter == 0) {
        _endTransmissionCounter = _endTransmissionTogglePeriod;
        _endTransmissionValue = 1 - _endTransmissionValue;
    }
    _endTransmissionCounter--;
    return _endTransmissionValue;
}

int TwoWire::read() {
    _available--;
    if (_flatline) {
        return _nextResult;
    }
    return _nextResult++;
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t size, bool stop) {
    return 0;
}

uint8_t TwoWire::getAddress() {
    return _address;
}

size_t TwoWire::write(uint8_t value) {
    if (_writeIndex < WriteBufferSize) {
        _written[_writeIndex++] = value;
        return true;
    }
    return false;
}

short TwoWire::writeMismatchIndex(const uint8_t* expected, const short length) const {
    if (length != _writeIndex) return -1;
    for (short i = 0; i < length; i++) {
        if (_written[i] != *expected) return i;
        expected++;
    }
    return length;
}

void TwoWire::setFlatline(bool flatline, uint8_t value) {
    _flatline = flatline;
    _nextResult = value;
}

void TwoWire::setEndTransmissionTogglePeriod(int period) {
    _endTransmissionTogglePeriod = period;
    _endTransmissionCounter = period;
    _endTransmissionValue = 0;
}
