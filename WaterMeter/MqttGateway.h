// Copyright 2021-2024 Rik Essenius
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

#ifndef HEADER_MQTT_GATEWAY
#define HEADER_MQTT_GATEWAY

#include <ESP.h>
#include <PubSubClient.h>

#include "Configuration.h"
#include "DataQueue.h"
#include "WiFiClientFactory.h"

namespace WaterMeter {
#define CALLBACK_SIGNATURE std::function<void(char*, char*)>

    constexpr auto Empty = "";
    constexpr auto DeviceLabel = "device";
    constexpr auto DeviceFreeHeap = "free-heap";
    constexpr auto DeviceFreeStack = "free-stack";
    constexpr auto DeviceFreeQueueSize = "free-queue-size";
    constexpr auto DeviceFreeQueueSpaces = "free-queue-spaces";
    constexpr auto DeviceBuild = "firmware-version";
    constexpr auto DeviceMac = "mac-address";
    constexpr auto DeviceResetSensor = "reset-sensor";
    constexpr auto Measurement = "measurement";
    constexpr auto MeasurementBatchSize = "batch-size";
    constexpr auto MeasurementBatchSizeDesired = "batch-size-desired";
    constexpr auto MeasurementValues = "values";
    constexpr auto Result = "result";
    constexpr auto ResultIdleRate = "idle-rate";
    constexpr auto ResultNonIdleRate = "non-idle-rate";
    constexpr auto ResultRate = "rate";
    constexpr auto ResultMeter = "meter";
    constexpr auto ResultValues = "values";
    constexpr auto State = "$state";

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
        static constexpr int PayloadBufferSize = 100;
        PubSubClient* _mqttClient;
        WiFiClientFactory* _wifiClientFactory;
        WiFiClient* _wifiClient = nullptr;
        const MqttConfig* _mqttConfig;
        const DataQueue* _dataQueue;
        int _announceIndex = 0;
        bool _justStarted = true;
        char _announcementBuffer[AnnouncementBufferSize] = {};
        char* _announcementPointer = _announcementBuffer;
        const char* _buildVersion;
        const char* _clientName = nullptr;
        unsigned long _reconnectTimestamp = 0UL;
        char _topicBuffer[TopicBufferSize] = {};
        char _meterPayload[PayloadBufferSize] = "";
        bool _meterPayloadReceived = false;

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
        void publishToMqtt(Topic topic, const char* payload);
    };
}
#endif