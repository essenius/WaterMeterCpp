// Copyright 2022-2023 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

// Button driver with debounce mechanism

#ifndef HEADER_BUTTON
#define HEADER_BUTTON

#include <ESP.h>
#include "ChangePublisher.h"

namespace WaterMeter {

    class Button {
    public:
        Button(ChangePublisher<uint8_t>* publisher, const uint8_t port, const unsigned long durationMillis = 10, const uint8_t  pinMode = INPUT_PULLUP) :
            _state(publisher), _port(port), _duration(durationMillis), _pinMode(pinMode) {}
        void begin();
        void check();
    private:
        ChangePublisher<uint8_t>* _state;
        uint8_t _port;
        unsigned long _duration;
        uint8_t _pinMode;
        int _lastState = LOW;
        unsigned long _lastSwitchTime = 0;
        uint8_t _currentState = LOW;
    };
}
#endif