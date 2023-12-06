// Copyright 2021-2023 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

// ReSharper disable CppClangTidyReadabilitySuspiciousCallArgument -- false positive on LED_BLUE

#include <ESP.h>

#include "LedDriver.h"
#include "ConnectionState.h"
#include <cstdint>
#include "Led.h"

LedDriver::LedDriver(EventServer* eventServer) :
    EventClient(eventServer),
    _connectingFlasher(Led::Aux, ConnectingInterval),
    _sampleFlasher(Led::Running, IdleInterval) {
    eventServer->subscribe(this, Topic::Begin);
}

void LedDriver::begin() {
    _eventServer->subscribe(this, Topic::Alert);
    _eventServer->subscribe(this, Topic::Blocked);
    _eventServer->subscribe(this, Topic::Connection);
    _eventServer->subscribe(this, Topic::ConnectionError);
    _eventServer->subscribe(this, Topic::Anomaly);
    _eventServer->subscribe(this, Topic::SensorState);
    _eventServer->subscribe(this, Topic::Pulse);
    _eventServer->subscribe(this, Topic::ResultWritten);
    _eventServer->subscribe(this, Topic::Sample);
    _eventServer->subscribe(this, Topic::TimeOverrun);
    Led::init(Led::Aux, Led::Off);
    Led::init(Led::Blue, Led::Off);
    Led::init(Led::Green, Led::Off);
    Led::init(Led::Red, Led::Off);
    Led::init(Led::Running, Led::Off);
    Led::init(Led::Yellow, Led::Off);
}

void LedDriver::update(const Topic topic, const char* payload) {
    // red led means an error condition. It can have multiple causes (see below as well)
    if (topic == Topic::ConnectionError) {
        Led::set(Led::Red, Led::On);
    }
}

void LedDriver::connectionUpdate(const ConnectionState payload) {
    switch (payload) {
    case ConnectionState::Disconnected:
        _connectingFlasher.reset();
        Led::set(Led::Aux, Led::Off);
        Led::set(Led::Blue, Led::Off);
        return;
    case ConnectionState::MqttReady:
        _connectingFlasher.reset();
        Led::set(Led::Aux, Led::On);
        Led::set(Led::Blue, Led::Off);
        return;
    case ConnectionState::CheckFirmware:
        Led::set(Led::Blue, Led::On);
        _connectingFlasher.signal();
        return;
    default:
        _connectingFlasher.signal();
    }
}

void LedDriver::timeOverrunUpdate(const bool isOn) {
    if (isOn) {
        Led::set(Led::Red, Led::On);
        Led::set(Led::Blue, Led::On);
    }
    else {
        Led::set(Led::Red, Led::Off);
        Led::set(Led::Blue, Led::Off);
    }
}

// ReSharper disable once CyclomaticComplexity - simple case statement
void LedDriver::update(const Topic topic, long payload) {
    const uint8_t state = payload ? Led::On : Led::Off;
    switch (topic) {
    case Topic::Begin:
        // do this as early as possible as sensors might need to signal issues
        if (payload == false) {
            begin();
        }
        break;
    // no connection causes a flashing green led, and blue while firmware is checked
    case Topic::Alert:
        Led::set(Led::Red, state);
        Led::set(Led::Green, state);
        return;
    case Topic::Anomaly:
        _sampleFlasher.setInterval(payload ? ExcludeInterval : IdleInterval);
        return;
    case Topic::Blocked:
        Led::set(Led::Red, state);
        return;
    case Topic::Connection:
        connectionUpdate(static_cast<ConnectionState>(payload));
        return;
    case Topic::SensorState:
        Led::set(Led::Red, Led::On);
        break;
    case Topic::Pulse:
        Led::set(Led::Blue, state);
        break;
    case Topic::ResultWritten:
        Led::set(Led::Green, Led::Off);
        Led::set(Led::Red, Led::Off);
        Led::set(Led::Blue, Led::Off);
        return;
    case Topic::Sample:
        _sampleFlasher.signal();
        return;
    case Topic::TimeOverrun:
        timeOverrunUpdate(payload > 0);
        break;
    default:
        break;
    }
}
