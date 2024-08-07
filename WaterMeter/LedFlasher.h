// Copyright 2021-2024 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#ifndef HEADER_LED_FLASHER
#define HEADER_LED_FLASHER

#include <ESP.h>

namespace WaterMeter {
    class LedFlasher {
    public:
        LedFlasher(uint8_t led, unsigned int interval);
        void reset();
        void setInterval(unsigned int interval);
        void signal();
    private:
        unsigned int _interval;
        uint8_t _led;
        unsigned int _ledCounter = 0;
    };
}
#endif
