// Copyright 2021-2022 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#include "LedFlasher.h"
#include "Led.h"

LedFlasher::LedFlasher(const uint8_t led, const unsigned int interval) : _interval(interval), _led(led) {}

// since it's flashing anyway, it doesn't really matter if the led is on or off with HIGH

void LedFlasher::reset() {
    _ledCounter = 0;
    Led::set(_led, Led::OFF);
}

void LedFlasher::setInterval(const unsigned int interval) {
    _interval = interval;
    reset();
}

void LedFlasher::signal() {
    if (_ledCounter == 0) {
        Led::toggle(_led);
        _ledCounter = _interval;
    }
    _ledCounter--;
}
