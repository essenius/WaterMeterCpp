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

// Mock implementation for if we are not targeting the Arduino

#ifndef ESP32

    #ifndef HEADER_PUBSUBCLIENT_MOCK
    #define HEADER_PUBSUBCLIENT_MOCK

    #include <functional>
    #include "NetMock.h"

#define MQTT_CALLBACK_SIGNATURE std::function<void(char*, uint8_t*, unsigned int)> callback

    class PubSubClient {
	public:
        PubSubClient& setClient(Client& client) { return *this; }
        bool setBufferSize(int size) { return true; }
        PubSubClient& setServer(const char* broker, const int port) { return *this; }
        PubSubClient& setCallback(MQTT_CALLBACK_SIGNATURE);
        bool connect(const char* id);
        bool connect(const char* id, const char* user, const char* pass);
        bool connected() { return _canConnect;  }
        bool subscribe(const char* topic) { return _canSubscribe; }
        bool loop() { return true; }
        bool publish(const char* topic, const char* payload);
        int state() { return 3; }

        // test assitance functions
        void setCanConnect(bool canConnect) { _canConnect = canConnect; }
        void setCanSubscribe(bool canSubscribe) { _canSubscribe = canSubscribe; }
        void setCanPublish(bool canPublish) { _canPublish = canPublish; }
        const char* getTopics() const { return _topic; }
        const char* getPayloads() const { return _payload; }
        int getCallCount() const { return _callCount; }
        void reset();
        void callBack(char* topic, uint8_t* payload, unsigned int size) { _callback(topic, payload, size);  }
        const char* id() const { return _id;  }
        const char* user() const { return _user; }

    private:
        constexpr static int TOPIC_SIZE = 2048;
        constexpr static int PAYLOAD_SIZE = 1024;
        constexpr static int FIELD_SIZE = 64;
        bool _canConnect = true;
        bool _canSubscribe = true;
        bool _canPublish = true;

        char _topic[TOPIC_SIZE] = {};
        char _payload[PAYLOAD_SIZE] = {};
        int _callCount = 0;
        char _user[FIELD_SIZE] = {};
        char _id[FIELD_SIZE] = {};
        char _pass[FIELD_SIZE] = {};
        std::function<void(char*, unsigned char*, unsigned)> _callback;
    };

    #endif
#endif