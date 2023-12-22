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

// Button with a debouncing algorithm. Uses a change publisher to pass on the button state.

#include "Button.h"

namespace WaterMeter {
    void Button::begin() {
        pinMode(_port, _pinMode);
        *_state = digitalRead(_port);
        _lastState = *_state;
        _lastSwitchTime = 0;
    }

    void Button::check() {
        _currentState = digitalRead(_port);
        // if the state changed, start the timer
        if (_currentState != _lastState) {
            _lastSwitchTime = millis();
            _lastState = _currentState;
        }

        // if we waited at least the duration and the button changed state, pass it on
        if (millis() - _lastSwitchTime > _duration && _currentState != *_state) {
            *_state = _currentState;
        }
    }
}