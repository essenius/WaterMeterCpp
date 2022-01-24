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

/* Init = 0, Disconnected,
WifiConnecting, WifiConnected, WifiReady,
MqttConnecting, WaitingForMqttReconnect, MqttConnected, MqttReady,
SetTime, SettingTime, CheckFirmware */

Log::Log(EventServer* eventServer) :
    EventClient(eventServer) {}
    //_disconnectedPublisher(eventServer, this, Topic::Error),
    //_connectedPublisher(eventServer, this, Topic::Info)


void Log::begin() {
    Serial.begin(115200);
    update(Topic::Info,"Starting");
    _eventServer->subscribe(this, Topic::Alert);
    _eventServer->subscribe(this, Topic::Connection);
    _eventServer->subscribe(this, Topic::Error);
    _eventServer->subscribe(this, Topic::Info);
    _eventServer->subscribe(this, Topic::TimeOverrun);
    _eventServer->subscribe(this, Topic::Result);

}

void Log::update(Topic topic, const char* payload) {
    const char* timestamp = _eventServer->request(Topic::Time, "");
    Serial.printf("[%s] ", timestamp);
    switch (topic) {
    case Topic::Connection:
        Serial.println(payload);
        break;
    case Topic::Error:
        Serial.printf("Error: %s\n", payload);
        break;
    case Topic::Info:
        Serial.println(payload);
        break;
    case Topic::Alert:
        Serial.println("Alert!");
        break;
    case Topic::TimeOverrun:
        Serial.printf("Time overrun: %s\n", payload);
        break;
    default:
        Serial.printf("Topic '%d': %s\n", static_cast<int>(topic), payload);
    }
}

void Log::update(Topic topic, const long payload) {
    if (topic == Topic::Connection) {
        update(topic, MESSAGES[payload]);
        return;
    }
    EventClient::update(topic, payload);
}
