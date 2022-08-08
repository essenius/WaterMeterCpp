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

#include "Meter.h"
#include <string>
#include <cmath>

#include "EventServer.h"
#include "SafeCString.h"


Meter::Meter(EventServer* eventServer) : EventClient(eventServer) {}

void Meter::begin() {
    _eventServer->subscribe(this, Topic::SetVolume);
    _eventServer->subscribe(this, Topic::Pulse);
}

const char* Meter::getVolume() {
    // trick to avoid rounding errors of .00049999999: take the next higher value
    const double volume = nextafter(_volume + _pulses * PULSE_DELTA, 1e7);
    safeSprintf(_buffer, "%013.7f", volume);
    return _buffer;
}

void Meter::newPulse() {
    _pulses++;
    publishValues();
}

void Meter::publishValues() {
    _eventServer->publish(Topic::Pulses, _pulses);
    _eventServer->publish(Topic::Volume, getVolume());
}

bool Meter::setVolume(const char* meterValue) {
    char* end;
    const auto volume = std::strtod(meterValue, &end);
    const auto allCharactersParsed = *end == '\0';
    if (allCharactersParsed) {
        _volume = volume;
        _pulses = 0;
        publishValues();
        return true;
    }
    return false;
}

void Meter::update(const Topic topic, const char* payload) {
    if (topic == Topic::SetVolume) {
        setVolume(payload);
    }
}

void Meter::update(const Topic topic, const long payload) {
    // we only register top and bottom extremes (values 1 and 3).
    // Since we have a tilted ellipse and not a circle, quarters are not reliable. Halves are.
    if (topic == Topic::Pulse && payload % 2 == 1) {
        newPulse();
    }
    else if (topic == Topic::SetVolume) {
        // this is done to minimize payload sizes for the queues (ints only).
        // A bit crude as it expects that addresses are 32 bits which is true on ESP32 but not on Win64.
        const auto volume = reinterpret_cast<char*>(payload);
        setVolume(volume);
    }
}
