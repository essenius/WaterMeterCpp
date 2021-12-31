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

#include "Log.h"

Log::Log(EventServer* eventServer) :
    EventClient("Log", eventServer),
    _disconnectedPublisher(eventServer, this, Topic::Error),
    _connectedPublisher(eventServer, this, Topic::Info) {}

void Log::begin() {
    Serial.begin(115200);
    Serial.println("Starting");
    _eventServer->subscribe(this, Topic::Connected);
    _eventServer->subscribe(this, Topic::Disconnected);
    _eventServer->subscribe(this, Topic::Error);
    _eventServer->subscribe(this, Topic::Info);
}

void Log::update(Topic topic, const char* payload) {
    const char* timestamp = _eventServer->request(Topic::Time, "");
    Serial.printf("[%s] ", timestamp);
    switch (topic) {
    case Topic::Error:
        if (payload == "") {
            Serial.println("Cleared error");
        }
        else {
            Serial.printf("Error: %s\n", payload);
        }
        break;
    case Topic::Info:
        Serial.printf("Info: %s\n", payload);
        break;
    case Topic::Connected:
        Serial.println("Connected");
        break;
    case Topic::Disconnected:
        Serial.println("Disconnected");
        break;
    default:
        Serial.printf("Topic '%d': %s\n", static_cast<int>(topic), payload);
    }
}


