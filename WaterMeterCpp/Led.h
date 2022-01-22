// Copyright 2022 Rik Essenius
// 
//   Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//   except in compliance with the License. You may obtain a copy of the License at
// 
//       http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software distributed under the License
//    is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and limitations under the License.

#ifndef HEADER_LED
#define HEADER_LED

#ifdef ESP32
#include <ESP.h>
#else
#include "ArduinoMock.h"
#endif

    // Wait = green (17) / flow = blue (16) exclude = red (18) -- all ok
    // conn = green (4) / upload = blue (5) / block = red (2) -- all OK
    // error = red (15) ok
    // send = blue (19) ok
    // overrun = yellow (18) ok                         15 2 4 16 17 5 18 19 21 22 23
    // peak = 23
    // sda = 21 -- reserved
    // scl = 22

   // static constexpr unsigned char RED_LED2 = 15;
   // static constexpr unsigned char GREEN_LED2 = 4;
   // static constexpr unsigned char BLUE_LED2 = 5;

class Led {
public:
    static void init(const uint8_t port, const uint8_t value = OFF);
    static void set(uint8_t port, uint8_t value);
    static void toggle(uint8_t port);
    static uint8_t get(uint8_t port);
    static constexpr unsigned char RUNNING = 23; //TODO: must become LED_BUILTIN
    static constexpr unsigned char AUX = 19;     // now red
    static constexpr unsigned char BLUE = 16;
    static constexpr unsigned char GREEN = 17;
    static constexpr unsigned char RED = 18;
    static constexpr  unsigned char YELLOW = 2; // overrun
    static constexpr unsigned char ERROR = 4; /// unused
    static constexpr uint8_t ON = HIGH;
    static constexpr uint8_t OFF = LOW;
private:
    static uint8_t convert(uint8_t port, uint8_t value);
    static bool needsInvert(uint8_t port);
};

#endif