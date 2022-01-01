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

#ifndef ESP32

#include "PubSubClientMock.h"
PubSubClient& PubSubClient::setCallback(MQTT_CALLBACK_SIGNATURE) {
    _callback = callback;
    return *this;
}

bool PubSubClient::connect(const char* id, const char* willTopic, uint8_t willQos, bool willRetain, const char* willMessage) {
    strcpy_s(_id, FIELD_SIZE, id);
    return _canConnect;
}

bool PubSubClient::connect(const char* id, const char* user, const char* pass,
    const char* willTopic, uint8_t willQos, bool willRetain, const char* willMessage) {
    strcpy_s(_user, FIELD_SIZE,  user);
    strcpy_s(_pass, FIELD_SIZE, pass);
    return connect(id, willTopic, willQos, willRetain, willMessage);
}

bool PubSubClient::publish(const char* topic, const char* payload, bool retain) {
    if (strlen(topic) + strlen(_topic) > TOPIC_SIZE - 1) return false;
    strcat_s(_topic, TOPIC_SIZE, topic);
    strcat_s (_topic, TOPIC_SIZE, "\n");
    if (strlen(payload) + strlen(_payload) > PAYLOAD_SIZE - 1) return false;
    strcat_s(_payload, PAYLOAD_SIZE, payload);
    if (!retain) strcat_s(_payload, PAYLOAD_SIZE, "[x]");
    strcat_s(_payload, PAYLOAD_SIZE, "\n");
    _callCount++;
    return _canPublish;
}

void PubSubClient::reset() {
    _callCount = 0;
    _payload[0] =  0;
    _topic[0] = 0;
    _user[0] = 0;
    _pass[0] = 0;
    _id[0] = 0;
    _canConnect = true;
    _canSubscribe = true;
    _canPublish = true;

}

#endif
