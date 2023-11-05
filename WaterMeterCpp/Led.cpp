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

#include "Led.h"

inline uint8_t Led::convert(const uint8_t port, const uint8_t value) {
    return needsInvert(port) ? !value : value;
}

uint8_t Led::get(const uint8_t port) {
    const auto result = digitalRead(port);
    return convert(port, result);
}

void Led::init(const uint8_t port, const uint8_t value) {
    pinMode(port, OUTPUT);
    set(port, value);
}

inline bool Led::needsInvert(const uint8_t port) {
    // the RGB led is common anode, so off HIGH
    return port == Red || port == Green || port == Blue;
}

void Led::set(const uint8_t port, const uint8_t value) {
    digitalWrite(port, convert(port, value));
}

void Led::toggle(const uint8_t port) {
    digitalWrite(port, !digitalRead(port));
}
