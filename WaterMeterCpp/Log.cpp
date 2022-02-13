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

#include "Log.h"

#include "ConnectionState.h"

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
    "Checking for firmware update" };

Log::Log(EventServer* eventServer, PayloadBuilder* wifiPayloadBuilder) :
    EventClient(eventServer), _wifiPayloadBuilder(wifiPayloadBuilder) {}

void Log::begin() {
    update(Topic::MessageFormatted,"Starting");
    _eventServer->subscribe(this, Topic::Alert);
    _eventServer->subscribe(this, Topic::Blocked);
    _eventServer->subscribe(this, Topic::Connection);
    _eventServer->subscribe(this, Topic::FreeHeap);
    _eventServer->subscribe(this, Topic::FreeStackSampler);
    _eventServer->subscribe(this, Topic::FreeStackCommunicator);
    _eventServer->subscribe(this, Topic::FreeStackConnector);
    _eventServer->subscribe(this, Topic::FreeQueue);
    _eventServer->subscribe(this, Topic::MessageFormatted);
    _eventServer->subscribe(this, Topic::ResultFormatted);
    _eventServer->subscribe(this, Topic::ResultWritten);
    _eventServer->subscribe(this, Topic::SensorWasReset);
    _eventServer->subscribe(this, Topic::TimeOverrun);
    _eventServer->subscribe(this, Topic::WifiSummaryReady);
}

void Log::printTimestamp() const {
    const char* timestamp = _eventServer->request(Topic::Time, "");
    Serial.printf("[%s] ", timestamp);
}

void Log::update(Topic topic, const char* payload) {
    printTimestamp();
    switch (topic) {
    case Topic::Alert:
        Serial.println("Alert!");
        break;
    case Topic::Blocked:
        Serial.printf("Blocked: %s\n", payload);
        break;
    case Topic::Connection:
        Serial.println(payload);
        break;
    case Topic::ConnectionError:
        Serial.printf("Error: '%s'\n", payload);
        break;
    case Topic::FreeHeap:
        Serial.printf("Free Heap: %s\n", payload);
        break;
    case Topic::FreeStackSampler:
        Serial.printf("Free Stack Sampler: %s\n", payload);
        break;
    case Topic::FreeStackCommunicator:
        Serial.printf("Free Stack Communicator: %s\n", payload);
        break;
    case Topic::FreeStackConnector:
        Serial.printf("Free Stack Connector: %s\n", payload);
        break;
    case Topic::MessageFormatted:
        Serial.println(payload);
        break;
    case Topic::ResultFormatted:
        Serial.printf("Result: %s\n", payload);
        break;
    case Topic::ResultWritten:
        Serial.printf("Result Written: %s\n", payload);
        break;
    case Topic::SensorWasReset:
        Serial.println("Sensor was reset");
        break;
    case Topic::TimeOverrun:
        Serial.printf("Time overrun: %s\n", payload);
        break;
    case Topic::WifiSummaryReady:
        Serial.printf("Wifi summary: %s\n", _wifiPayloadBuilder->toString());
        break;
    default:
        Serial.printf("Topic '%d': %s\n", static_cast<int>(topic), payload);
    }
}

void Log::update(Topic topic, const long payload) {
    if (topic == Topic::Connection) {
      if (_previousConnectionTopic != payload) {
        _previousConnectionTopic = payload;
        update(topic, MESSAGES[payload]);
      }
      return;
    }
    if (topic == Topic::FreeQueue) {
        printTimestamp();
        const int index = payload >> 24;
        const long value = payload & 0x00FFFFFF;
        Serial.printf("Free DataQueue #%d space: %ld\n", index, value);
        return;
    }
    EventClient::update(topic, payload);
}
