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

// Manages the MQTT interface using Homie protocol.
// * Creates the connection
// * Announces the topics
// * Listens to events and translates those to MQTT messages to be sent to the broker,
// * Listens to MQTT and translates understood incoming messages to events.

#ifndef HEADER_MQTTGATEWAY
#define HEADER_MQTTGATEWAY

#include <ESP.h>
#include <PubSubClient.h>

#include "Configuration.h"
#include "DataQueue.h"
#include "WiFiClientFactory.h"

namespace WaterMeter {
#define CALLBACK_SIGNATURE std::function<void(char*, char*)>

    constexpr const char* const Empty = "";
    constexpr const char* const DeviceLabel = "device";
    constexpr const char* const DeviceFreeHeap = "free-heap";
    constexpr const char* const DeviceFreeStack = "free-stack";
    constexpr const char* const DeviceFreeQueueSize = "free-queue-size";
    constexpr const char* const DeviceFreeQueueSpaces = "free-queue-spaces";
    constexpr const char* const DeviceBuild = "firmware-version";
    constexpr const char* const DeviceMac = "mac-address";
    constexpr const char* const DeviceResetSensor = "reset-sensor";
    constexpr const char* const Measurement = "measurement";
    constexpr const char* const MeasurementBatchSize = "batch-size";
    constexpr const char* const MeasurementBatchSizeDesired = "batch-size-desired";
    constexpr const char* const MeasurementValues = "values";
    constexpr const char* const Result = "result";
    constexpr const char* const ResultIdleRate = "idle-rate";
    constexpr const char* const ResultNonIdleRate = "non-idle-rate";
    constexpr const char* const ResultRate = "rate";
    constexpr const char* const ResultMeter = "meter";
    constexpr const char* const ResultValues = "values";
    constexpr const char* const State = "$state";

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
        bool getPreviousVolume();
        bool handleQueue();
        virtual bool hasAnnouncement();
        virtual bool isConnected();
        virtual bool publishNextAnnouncement();
        using EventClient::update;
        void update(Topic topic, const char* payload) override;

    protected:
        static constexpr int TopicBufferSize = 255;
        static constexpr int AnnouncementBufferSize = 2500;
        static constexpr int NumberBufferSize = 20;
        PubSubClient* _mqttClient;
        WiFiClientFactory* _wifiClientFactory;
        WiFiClient* _wifiClient = nullptr;
        const MqttConfig* _mqttConfig;
        const DataQueue* _dataQueue;
        int _announceIndex = 0;
        bool _justStarted = true;
        char _announcementBuffer[AnnouncementBufferSize] = { 0 };
        char* _announcementPointer = _announcementBuffer;
        const char* _buildVersion;
        const char* _clientName = nullptr;
        unsigned long _reconnectTimestamp = 0UL;
        char _topicBuffer[TopicBufferSize] = { 0 };
        char _volume[NumberBufferSize] = "";
        bool _volumeReceived = false;

        void callback(const char* topic, const byte* payload, unsigned length);
        static bool isRightTopic(std::pair<const char*, const char*> topicPair, const char* expectedNode, const char* expectedProperty);
        void prepareAnnouncementBuffer();
        void prepareEntity(const char* entity, const char* payload);
        void prepareEntity(const char* baseTopic, const char* entity, const char* payload);
        void prepareItem(const char* item);
        void prepareNode(const char* node, const char* name, const char* type, const char* properties);
        void prepareProperty(const char* node, const char* property, const char* attribute,
            const char* dataType, const char* format = Empty, bool settable = false);

        bool publishEntity(const char* baseTopic, const char* entity, const char* payload, bool retain = true);
        void publishError(const char* message);
        bool publishProperty(const char* node, const char* property, const char* payload, bool retain = true);
        void publishToEventServer(Topic topic, const char* payload);
        void publishUpdate(Topic topic, const char* payload);
    };
}
#endif