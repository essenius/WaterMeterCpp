// Copyright 2021-2022 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#include "Log.h"

#include <ESP.h>

SemaphoreHandle_t Log::_printMutex = xSemaphoreCreateMutex();

// expects the same size and order as the ConnectionState enum
constexpr static const char* const MESSAGES[] = {
    "Initializing connection",
    "Disconnected",
    "Connecting to Wifi",
    "Connected to Wifi",
    "Wifi ready",
    "Connecting to MQTT broker",
    "Waiting to reconnect to MQTT broker",
    "Connected to MQTT broker",
    "MQTT ready",
    "Requesting time",
    "Waiting for time",
    "Checking for firmware update"
};

Log::Log(EventServer* eventServer, PayloadBuilder* wifiPayloadBuilder) :
    EventClient(eventServer), _wifiPayloadBuilder(wifiPayloadBuilder) {
    eventServer->subscribe(this, Topic::Begin);
}

void Log::begin() {
    update(Topic::MessageFormatted, "Starting");
    _eventServer->subscribe(this, Topic::Alert);
    _eventServer->subscribe(this, Topic::Blocked);
    _eventServer->subscribe(this, Topic::Connection);
    _eventServer->subscribe(this, Topic::ErrorFormatted);
    _eventServer->subscribe(this, Topic::FreeHeap);
    _eventServer->subscribe(this, Topic::FreeStack);
    _eventServer->subscribe(this, Topic::FreeQueueSize);
    _eventServer->subscribe(this, Topic::FreeQueueSpaces);
    _eventServer->subscribe(this, Topic::MessageFormatted);
    _eventServer->subscribe(this, Topic::NoDisplayFound);
    _eventServer->subscribe(this, Topic::NoFit);
    _eventServer->subscribe(this, Topic::NoSensorFound);
    _eventServer->subscribe(this, Topic::ResultFormatted);
    _eventServer->subscribe(this, Topic::SensorWasReset);
    _eventServer->subscribe(this, Topic::SetVolume);
    _eventServer->subscribe(this, Topic::SkipSamples);
    _eventServer->subscribe(this, Topic::TimeOverrun);
    _eventServer->subscribe(this, Topic::UpdateProgress);
    _eventServer->subscribe(this, Topic::WifiSummaryReady);
}

const char* Log::getTimestamp() const {
    return _eventServer->request(Topic::Time, "");
}

// ReSharper disable once CyclomaticComplexity - just a case statement
void Log::update(Topic topic, const char* payload) {
    switch (topic) {
    case Topic::Alert:
        log("Alert: %s", payload);
        break;
    case Topic::Blocked:
        log("Blocked: %s", payload);
        break;
    case Topic::Connection: 
    case Topic::ErrorFormatted:
    case Topic::MessageFormatted:
        log("%s", payload);
        break;
    case Topic::FreeHeap:
        log("Free Heap: %s", payload);
        break;
    case Topic::NoDisplayFound:
        log("No OLED display found");
        break;
    case Topic::NoSensorFound:
        log("No sensor found: %s", payload);
        break;
    case Topic::ResultFormatted:
        log("Result: %s", payload);
        break;
    case Topic::SensorWasReset:
        log("Sensor was reset: %s", payload);
        break;
    case Topic::SetVolume:
        log("Set meter volume: %s", payload);
        break;
    case Topic::SkipSamples:
        log("Skipped %s samples", payload);
        break;
    case Topic::TimeOverrun:
        log("Time overrun: %s", payload);
        break;
    case Topic::UpdateProgress:
        log("Firmware update progress: %s%%", payload);
        break;
    case Topic::WifiSummaryReady:
        log("Wifi summary: %s", _wifiPayloadBuilder->toString());
        break;
    default:
        log("Topic '%d': %s", static_cast<int>(topic), payload);
    }
}

void Log::update(const Topic topic, const long payload) {
    switch (topic) {
    case Topic::Begin:
        // do this as early as possible, i.e. when called with LONG_FALSE
        if (payload == LONG_FALSE) {
            begin();
            }
        break;
    case Topic::Connection:
        if (_previousConnectionTopic != payload) {
            _previousConnectionTopic = payload;
            update(topic, MESSAGES[payload]);
        }
        return;
    case Topic::FreeQueueSize:
        printIndexedPayload("Memory DataQueue", payload);
        return;
    case Topic::FreeQueueSpaces:
        printIndexedPayload("Spaces Queue", payload);
        return;
    case Topic::FreeStack:
        printIndexedPayload("Stack", payload);
        return;
    case Topic::NoFit:
        log("No fit: %ld deg", payload);
        return;
    default:
        EventClient::update(topic, payload);
    }
}

void Log::printIndexedPayload(const char* entity, const long payload) const {
    const int index = payload >> 24;
    const long value = payload & 0x00FFFFFF;
    log("Free %s #%d: %ld", entity, index, value);
}
