// Copyright 2021 Rik Essenius
// 
//   Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//   except in compliance with the License. You may obtain a copy of the License at
// 
//       http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software distributed under the License
//    is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and limitations under the License.

#ifdef ESP32
#include <ESP.h>
#else 
#include "ArduinoMock.h"
#endif

#include "LedDriver.h"
#include <stdint.h>
#include <string.h>

LedDriver::LedDriver(EventServer* eventServer) : EventClient("LedDriver", eventServer),
    _connectingFlasher(GREEN_LED, 2),
    _sampleFlasher(LED_BUILTIN, IDLE_INTERVAL) {}

void LedDriver::begin() {
    _eventServer->subscribe(this, Topic::Connected);
    _eventServer->subscribe(this, Topic::Connecting);
    _eventServer->subscribe(this, Topic::Disconnected);
    _eventServer->subscribe(this, Topic::Error);
    _eventServer->subscribe(this, Topic::Exclude);
    _eventServer->subscribe(this, Topic::Flow);
    _eventServer->subscribe(this, Topic::Sample);
    _eventServer->subscribe(this, Topic::Peak);
    _eventServer->subscribe(this, Topic::Sending);
    _eventServer->subscribe(this, Topic::TimeOverrun);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(AUX_LED, OUTPUT);
    pinMode(RED_LED, OUTPUT);
    pinMode(GREEN_LED, OUTPUT);
    pinMode(BLUE_LED, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);;
    digitalWrite(AUX_LED, LOW);
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
}

/// <summary>
/// If input is empty, return LOW. If it is more than one character, return HIGH.
/// this allows for using empty string vs message to switch leds.
/// If the input is one character, then return HIGH if it is one of 1,H,h,T,t and LOW otherwise.
/// </summary>
/// <param name="state">input string</param>
/// <returns>LOW or HIGH</returns>
uint8_t LedDriver::convertToState(const char* state) {
    if (strlen(state) == 0) return LOW;
    if (strlen(state) > 1) return HIGH;
    switch (state[0]) {
    case '0':
    case 'L':
    case 'l':
    case 'F':
    case 'f':
        return LOW;
    case '1':
    case 'H':
    case 'h':
    case 'T':
    case 't':
        return HIGH;
    default:
        return LOW;
    }
}

/// <summary>
/// Event listener, switching leds / updating flash rate
/// </summary>
/// <param name="topic"></param>
/// <param name="payload"></param>
void LedDriver::update(Topic topic, const char* payload) {
    uint8_t state = convertToState(payload);
    unsigned char led;
    switch (topic) {
    case Topic::Exclude:
    case Topic::Flow:
        update(topic, state);
        return;
    case Topic::Error:
        led = RED_LED;
        break;
    case Topic::Connected:
        led = GREEN_LED;
        state = true;
        break;
    case Topic::Connecting:
        _connectingFlasher.signal();
        return;
    case Topic::Disconnected:
        led = GREEN_LED;
        state = false;
        break;
    case Topic::Sending:
        led = AUX_LED;
        break;
    case Topic::TimeOverrun:
        led = RED_LED;
        break;

    case Topic::Peak:
        led = BLUE_LED;
        break;
    case Topic::Sample:
        _sampleFlasher.signal();
        // fall through to default
    default:
        return;
    }
    digitalWrite(led, state);
}

void LedDriver::update(Topic topic, long payload) {
    if (topic == Topic::Flow || topic == Topic::Exclude) {
        unsigned int interval = IDLE_INTERVAL;
        if (payload) {
            interval = topic == Topic::Flow ? FLOW_INTERVAL : EXCLUDE_INTERVAL;
        }
        _sampleFlasher.setInterval(interval);
        return;
    }
    update(topic, payload ? "1" : "0");
}
