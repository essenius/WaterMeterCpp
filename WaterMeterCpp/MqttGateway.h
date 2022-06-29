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

#ifndef HEADER_MQTTGATEWAY
#define HEADER_MQTTGATEWAY

#include <ESP.h>
#include <PubSubClient.h>

#include "Configuration.h"
#include "DataQueue.h"
#include "WiFiClientFactory.h"

#define CALLBACK_SIGNATURE std::function<void(char*, char*)>

constexpr const char* const EMPTY = "";
constexpr const char* const DEVICE = "device";
constexpr const char* const DEVICE_FREE_HEAP = "free-heap";
constexpr const char* const DEVICE_FREE_STACK = "free-stack";
constexpr const char* const DEVICE_FREE_QUEUE_SIZE = "free-queue-size";
constexpr const char* const DEVICE_FREE_QUEUE_SPACES = "free-queue-spaces";
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
constexpr const char* const RESULT_METER = "meter";
constexpr const char* const RESULT_VALUES = "values";
constexpr const char* const STATE = "$state";

class MqttGateway : public EventClient {
public:
    MqttGateway(
        EventServer* eventServer,
        PubSubClient* mqttClient,
        WiFiClientFactory* wifiClientFactory,
        const MqttConfig* mqttConfig,
        const DataQueue* dataQueue,
        const char* buildVersion);
    MqttGateway(const MqttGateway&) = default;
    MqttGateway(MqttGateway&&) = default;
    MqttGateway& operator=(const MqttGateway&) = default;
    MqttGateway& operator=(MqttGateway&&) = default;
    ~MqttGateway() override;

    virtual void announceReady();
    virtual void begin(const char* clientName);
    virtual void connect();
    bool handleQueue();
    virtual bool hasAnnouncement();
    virtual bool isConnected();
    void publishError(const char* message);
    virtual bool publishNextAnnouncement();
    using EventClient::update;
    void update(Topic topic, const char* payload) override;
    void publishUpdate(Topic topic, const char* payload);

protected:
    static constexpr int TOPIC_BUFFER_SIZE = 255;
    static constexpr int ANNOUNCEMENT_BUFFER_SIZE = 2500;
    static constexpr int NUMBER_BUFFER_SIZE = 20;
    PubSubClient* _mqttClient;
    WiFiClientFactory* _wifiClientFactory;
    WiFiClient* _wifiClient = nullptr;
    const MqttConfig* _mqttConfig;
    const DataQueue* _dataQueue;
    int _announceIndex = 0;
    char _announcementBuffer[ANNOUNCEMENT_BUFFER_SIZE] = {0};
    char* _announcementPointer = _announcementBuffer;
    const char* _buildVersion;
    const char* _clientName = nullptr;
    unsigned long _reconnectTimestamp = 0UL;
    char _topicBuffer[TOPIC_BUFFER_SIZE] = {0};
    char _volume[NUMBER_BUFFER_SIZE] = "";

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
