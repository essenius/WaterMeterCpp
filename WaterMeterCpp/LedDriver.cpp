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

#ifdef ESP32
#include <ESP.h>
#else
#include "ArduinoMock.h"
#endif

#include "LedDriver.h"
#include "ConnectionState.h"
#include <cstdint>
#include "Led.h"

LedDriver::LedDriver(EventServer* eventServer) :
    EventClient(eventServer),
    _connectingFlasher(Led::GREEN, CONNECTING_INTERVAL),
    _sampleFlasher(Led::RUNNING, IDLE_INTERVAL) {}

void LedDriver::begin() {
    _eventServer->subscribe(this, Topic::Alert);
    _eventServer->subscribe(this, Topic::Blocked);
    _eventServer->subscribe(this, Topic::Connection);
    _eventServer->subscribe(this, Topic::SamplingError);
    _eventServer->subscribe(this, Topic::CommunicationError);
    _eventServer->subscribe(this, Topic::Exclude);
    _eventServer->subscribe(this, Topic::Flow);
    _eventServer->subscribe(this, Topic::Peak);
    _eventServer->subscribe(this, Topic::ResultWritten);
    _eventServer->subscribe(this, Topic::Sample);
    _eventServer->subscribe(this, Topic::TimeOverrun);
    Led::init(Led::RUNNING, Led::OFF);
    Led::init(Led::AUX, Led::OFF);
    Led::init(Led::YELLOW, Led::OFF);
    Led::init(Led::RED, Led::OFF);
    Led::init(Led::GREEN, Led::OFF);
    Led::init(Led::BLUE, Led::OFF);
}

void LedDriver::update(const Topic topic, const char* payload) {
    // red led has two functions: error and overrun
    if (topic == Topic::SamplingError || topic == Topic::CommunicationError) {
        Led::set(Led::RED, Led::ON);
    }
}

void LedDriver::update(Topic topic, long payload) {
    const uint8_t state = payload ? Led::ON : Led::OFF;
    switch (topic) {
    // flow and exclude change the flash rate 
    case Topic::Flow:
    case Topic::Exclude: {
        unsigned int interval = IDLE_INTERVAL;
        if (payload) {
            interval = topic == Topic::Flow ? FLOW_INTERVAL : EXCLUDE_INTERVAL;
        }
        _sampleFlasher.setInterval(interval);
        return;
    }
    // no connection causes a flashing green led, and blue while firmware is checked
    case Topic::Connection:
        switch (static_cast<ConnectionState>(payload)) {
        case ConnectionState::Disconnected:
            _connectingFlasher.reset();
            Led::set(Led::GREEN, Led::OFF);
            Led::set(Led::BLUE, Led::OFF);
            return;
        case ConnectionState::MqttReady:
            _connectingFlasher.reset();
            Led::set(Led::GREEN, Led::ON);
            Led::set(Led::BLUE, Led::OFF);
            return;
        case ConnectionState::CheckFirmware:
            Led::set(Led::BLUE, Led::ON);
            _connectingFlasher.signal();
            return;
        default:
            _connectingFlasher.signal();
        }
        return;
    case Topic::Sample:
        _sampleFlasher.signal();
        return;
    case Topic::Blocked:
    case Topic::Alert:
        Led::toggle(Led::RED);
        return;
    case Topic::ResultWritten:
        Led::toggle(Led::AUX);
        return;
    case Topic::TimeOverrun:
        Led::set(Led::YELLOW, state);
        return;
    case Topic::Peak:
        Led::set(Led::BLUE, state);
        break;
    default:
        break;
    }
}
