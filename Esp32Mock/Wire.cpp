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

#include "Wire.h"

TwoWire Wire;

int TwoWire::available() { return 6; }
int TwoWire::read() { return 0; }
void TwoWire::begin() {}
void TwoWire::beginTransmission(uint8_t address) {}
size_t TwoWire::write(uint8_t reg) { return 0; }
uint8_t TwoWire::endTransmission() { return 0; }
uint8_t TwoWire::requestFrom(uint8_t address, uint8_t size) { return 0; }
