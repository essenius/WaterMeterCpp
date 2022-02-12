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

// Mock implementation for if we are not targeting the Arduino
// ReSharper disable CppMemberFunctionMayBeStatic -- need to mimic existing interface
// ReSharper disable CppMemberFunctionMayBeConst -- same here

#ifndef ESP32

#ifndef HEADER_PUBSUBCLIENT_MOCK
#define HEADER_PUBSUBCLIENT_MOCK

#include <functional>
#include "NetMock.h"

#define MQTT_CALLBACK_SIGNATURE std::function<void(char*, uint8_t*, unsigned int)> callback

class PubSubClient {
public:
    bool connect(const char* id, const char* willTopic, uint8_t willQos, bool willRetain, const char* willMessage);
    bool connect(const char* id, const char* user, const char* pass, const char* willTopic, uint8_t willQos, bool willRetain,
        const char* willMessage);
    bool connected() { return _canConnect; }
    bool loop() {
        _loopCount++;
        return true;
    }
    bool publish(const char* topic, const char* payload, bool retain = false);
    bool setBufferSize(int size) { return true; }
    PubSubClient& setCallback(MQTT_CALLBACK_SIGNATURE);
    PubSubClient& setClient(Client& client) { return *this; }
    PubSubClient& setKeepAlive(uint16_t period) { return *this; }
    PubSubClient& setServer(const char* broker, const int port) { return *this; }
    bool subscribe(const char* topic) { return _canSubscribe; }
    int state() { return 3; }

    // test assistance functions
    void callBack(char* topic, uint8_t* payload, unsigned int size) { _callback(topic, payload, size); }
    int getCallCount() const { return _callCount; }
    int getLoopCount() const { return _loopCount; }
    const char* getPayloads() const { return _payload; }
    const char* getTopics() const { return _topic; }
    const char* id() const { return _id; }
    void reset();
    void setCanConnect(bool canConnect) { _canConnect = canConnect; }
    void setCanPublish(bool canPublish) { _canPublish = canPublish; }
    void setCanSubscribe(bool canSubscribe) { _canSubscribe = canSubscribe; }
    const char* user() const { return _user; }

private:
    constexpr static int FIELD_SIZE = 64;
    constexpr static int PAYLOAD_SIZE = 1024;
    constexpr static int TOPIC_SIZE = 2500;
    std::function<void(char*, unsigned char*, unsigned)> _callback;
    int _callCount = 0;
    bool _canConnect = true;
    bool _canPublish = true;
    bool _canSubscribe = true;
    char _id[FIELD_SIZE] = {};
    int _loopCount = 0;
    char _pass[FIELD_SIZE] = {};
    char _payload[PAYLOAD_SIZE] = {};
    char _topic[TOPIC_SIZE] = {};
    char _user[FIELD_SIZE] = {};
};

#endif
#endif
