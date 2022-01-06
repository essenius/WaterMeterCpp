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

#ifndef HEADER_MQTTGATEWAY
#define HEADER_MQTTGATEWAY
#include <map>
#include "EventServer.h"

#ifdef ESP32
#include "Client.h"
#else
#include "NetMock.h"
#include "PubSubClientMock.h"
#endif

#define CALLBACK_SIGNATURE std::function<void(char*, char*)>

constexpr const char* const EMPTY = "";
constexpr const char* const DEVICE = "device";
constexpr const char* const DEVICE_ERROR = "error";
constexpr const char* const DEVICE_FREE_HEAP = "free-heap";
constexpr const char* const DEVICE_FREE_STACK = "free-stack";
constexpr const char* const DEVICE_INFO = "info";
constexpr const char* const DEVICE_BUILD = "firmware-version";
constexpr const char* const DEVICE_MAC = "mac-address";
constexpr const char* const DEVICE_RESET_SENSOR = "reset-sensor";
constexpr const char* const MEASUREMENT = "measurement";
constexpr const char* const MEASUREMENT_BATCH_SIZE = "batch-size";
constexpr const char* const MEASUREMENT_BATCH_SIZE_DESIRED = "batch-size-desired";
constexpr const char* const MEASUREMENT_VALUES = "values";
constexpr const char* const RESULT = "result";
constexpr const char* const RESULT_IDLE_RATE = "idle-rate";
constexpr const char* const RESULT_NON_IDLE_RATE = "non-idle-rate";
constexpr const char* const RESULT_RATE = "rate";
constexpr const char* const RESULT_VALUES = "values";

static const std::map<Topic, std::pair<const char*, const char*>> TOPIC_MAP{
    {Topic::BatchSize, {MEASUREMENT, MEASUREMENT_BATCH_SIZE}},
    {Topic::BatchSizeDesired, {MEASUREMENT, MEASUREMENT_BATCH_SIZE_DESIRED}},
    {Topic::Measurement, {MEASUREMENT, MEASUREMENT_VALUES}},
    {Topic::Rate, {RESULT, RESULT_RATE}},
    {Topic::Result, {RESULT, RESULT_VALUES}},
    {Topic::IdleRate, {RESULT, RESULT_IDLE_RATE}},
    {Topic::NonIdleRate, {RESULT, RESULT_NON_IDLE_RATE}},
    {Topic::FreeHeap, {DEVICE, DEVICE_FREE_HEAP}},
    {Topic::FreeStack, {DEVICE, DEVICE_FREE_STACK}},
    {Topic::Error, {DEVICE, DEVICE_ERROR}},
    {Topic::Info, {DEVICE, DEVICE_INFO}},
    {Topic::Flatline, {DEVICE, DEVICE_RESET_SENSOR}}
};

class MqttGateway : public EventClient {
public:
    MqttGateway(EventServer* eventServer, const char* broker, int port, const char* user, const char* password,
                const char* buildVersion);
    virtual void announceReady();
    virtual void begin(Client* client, const char* clientName);
    virtual void connect();
    void handleQueue();
    virtual bool hasAnnouncement();
    virtual bool isConnected();
    void publishError(const char* message);
    virtual bool publishNextAnnouncement();
    void update(Topic topic, const char* payload) override;

protected:
    const char* _clientName = nullptr;
    unsigned long _reconnectTimestamp = 0UL;
    const char* _broker = nullptr;
    const char* _user;
    const char* _password;
    int _port = 1883;
    const char* _buildVersion;
    int _announceIndex = 0;
    static constexpr int TOPIC_BUFFER_SIZE = 255;
    char _topicBuffer[TOPIC_BUFFER_SIZE] = {0};
    static constexpr int ANNOUNCEMENT_BUFFER_SIZE = 2048; // using 1812 now.
    char _announcementBuffer[ANNOUNCEMENT_BUFFER_SIZE] = {0};
    char* _announcementPointer = _announcementBuffer;

    void callback(const char* topic, const byte* payload, unsigned length);

    void prepareAnnouncementBuffer();
    void prepareEntity(const char* entity, const char* payload);
    void prepareEntity(const char* baseTopic, const char* entity, const char* payload);
    void prepareItem(const char* item);
    void prepareNode(const char* node, const char* name, const char* type, const char* properties);
    void prepareProperty(const char* node, const char* property, const char* attribute,
        const char* dataType, const char* format = EMPTY, bool settable = false);

    bool publishEntity(const char* baseTopic, const char* entity, const char* payload, bool retain = true);
    bool publishProperty(const char* node, const char* property, const char* payload, bool retain = true);
};

#endif
