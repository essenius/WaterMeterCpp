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

#ifndef HEADER_LED
#define HEADER_LED

#include <ESP.h>

namespace WaterMeter {
    // Wait = green (17) / flow = blue (16) exclude = red (18) -- all ok
    // conn = green (4) / upload = blue (5) / block = red (2) -- all OK
    // error = red (15) ok
    // send = blue (19) ok
    // overrun = yellow (18) ok                         15 2 4 16 17 5 18 19 21 22 23
    // peak = 23
    // sda = 21 -- reserved
    // scl = 22

    class Led {
    public:
        static constexpr unsigned char Aux = 19; // now red
        static constexpr unsigned char Blue = 16;
        static constexpr unsigned char Error = 4; /// unused
        static constexpr unsigned char Green = 17;
        static constexpr unsigned char Red = 18;
        static constexpr unsigned char Running = LED_BUILTIN; // Was 23
        static constexpr unsigned char Yellow = 2; // unused
        static constexpr uint8_t On = HIGH;
        static constexpr uint8_t Off = LOW;

        static uint8_t get(uint8_t port);
        static void init(uint8_t port, uint8_t value = Off);
        static void set(uint8_t port, uint8_t value);
        static void toggle(uint8_t port);
    private:
        static uint8_t convert(uint8_t port, uint8_t value);
        static bool needsInvert(uint8_t port);
    };
}
#endif
