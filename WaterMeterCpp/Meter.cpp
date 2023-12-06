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

#include "Meter.h"
#include <string>
#include <cmath>

#include "EventServer.h"
#include <SafeCString.h>


Meter::Meter(EventServer* eventServer) : EventClient(eventServer) {
    eventServer->subscribe(this, Topic::Begin);
}

void Meter::begin() {
    _eventServer->subscribe(this, Topic::Pulse);
    _eventServer->subscribe(this, Topic::SetVolume);
    _eventServer->subscribe(this, Topic::AddVolume);
}

const char* Meter::getVolume() {
    // trick to avoid rounding errors of .00049999999: take the next higher value
    const double volume = nextafter(_volume + _pulses * PulseDelta, 1e7);
    SafeCString::sprintf(_buffer, "%013.7f", volume);
    return _buffer;
}

void Meter::newPulse() {
    _pulses++;
    publishValues();
}

void Meter::publishValues() {
    _eventServer->publish(Topic::Pulses, _pulses);
    _eventServer->publish(this, Topic::Volume, getVolume());
}

bool Meter::setVolume(const char* meterValue, const double addition) {
    char* end;
    const auto volume = std::strtod(meterValue, &end);
    const auto allCharactersParsed = *end == '\0';
    if (allCharactersParsed) {
        _volume = volume + addition;
        _pulses = 0;
        publishValues();
        return true;
    }
    return false;
}

void Meter::update(const Topic topic, const char* payload) {
    if (topic == Topic::SetVolume) {
        // if it was set via a set topic, don't add current value - this is a deliberate override
        setVolume(payload);
    } else if (topic == Topic::AddVolume) {
        // if it was set via a retained get topic, add volume that we found so far
        // This caters for any usage between the device boot and MQTT being up.
        // This happens just once, but we can't unsubscribe while we are iterating through the subscribers.
        // it is also not really necessary as the connector is guaranteed to send it only once.
        setVolume(payload, _volume);
    }
}

void Meter::update(const Topic topic, const long payload) {
    // we only register top and bottom extremes (values 1 and 3).
    // Since we have a tilted ellipse and not a circle, quarters are not reliable. Halves are.
    if (topic == Topic::Pulse && payload % 2 == 1) {
        newPulse();
    }

    // we initialize after the base services like logger and led driver were initialized
    else if (topic == Topic::Begin && payload) {
        begin();
    }
}
