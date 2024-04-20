// Copyright 2022-2024 Rik Essenius
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

namespace WaterMeter {
    Meter::Meter(EventServer* eventServer) : EventClient(eventServer) {
        eventServer->subscribe(this, Topic::Begin);
    }

    void Meter::begin() {
        _eventServer->subscribe(this, Topic::Pulse);
        _eventServer->subscribe(this, Topic::SetVolume);
        _eventServer->subscribe(this, Topic::AddVolume);
    }


    const char* Meter::extractVolume(const char* payload) {
        // The payload is a JSON string that has volume as its last entry. Extract the value.
        const auto partAfterLastColon = strrchr(payload, ':');
        if (partAfterLastColon == nullptr) return nullptr;

        size_t i = 0;
        while (i < strlen(partAfterLastColon) && partAfterLastColon[i + 1] != '}' && i <= BufferSize) {
            _jsonBuffer[i] = partAfterLastColon[i + 1];
            i++;
        }
        _jsonBuffer[i] = 0;
        return _jsonBuffer;
    }

    const char* Meter::getMeterPayload(const char* volume) {
        const char* timestamp = _eventServer->request(Topic::Time, "");
        SafeCString::sprintf(_jsonBuffer, R"({"timestamp":%s,"pulses":%d,"volume":%s})", timestamp, _pulses, volume);
        return _jsonBuffer;
    }

    const char* Meter::getVolume() {
        // trick to avoid rounding errors of .00049999999: take the next higher value
        const double volume = nextafter(_volume + _pulses * PulseDelta, 1e7);
        SafeCString::sprintf(_volumeBuffer, "%013.7f", volume);
        return _volumeBuffer;
    }

    void Meter::newPulse() {
        _pulses++;
        publishValues();
    }

    void Meter::publishValues() {
        _eventServer->publish(Topic::Pulses, _pulses);
        _eventServer->publish(Topic::Volume, getVolume());
        _eventServer->publish(this, Topic::MeterPayload, getMeterPayload(_volumeBuffer));
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
            // also, the set topic payload only contains the volume (not a json string)
            setVolume(payload);
        }
        else if (topic == Topic::AddVolume) {
            // if it was set via a retained get topic, add volume that we found so far
            // This caters for any usage between the device boot and MQTT being up.
            // This happens just once, but we can't unsubscribe while we are iterating through the subscribers.
            // it is also not really necessary as the connector is guaranteed to send it only once.
            setVolume(extractVolume(payload), _volume);
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
}
