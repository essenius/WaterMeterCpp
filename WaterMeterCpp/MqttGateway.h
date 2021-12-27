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

#ifndef HEADER_MQTTGATEWAY
#define HEADER_MQTTGATEWAY
#include <map>
#include "PubSub.h"
#include "BinaryStatusPublisher.h"
#include "secrets_mqtt.h"

#ifdef ESP32
#include "Client.h"
#else
#include "NetMock.h"
#include "PubSubClientMock.h"
#endif

#define CALLBACK_SIGNATURE std::function<void(char*, char*)>

const char* const DEVICE = "device";
const char* const DEVICE_ERROR = "error";
const char* const DEVICE_FREE_HEAP = "free-heap";
const char* const DEVICE_FREE_STACK = "free-stack";
const char* const DEVICE_INFO = "info";
const char* const MEASUREMENT = "measurement";
const char* const MEASUREMENT_BATCH_SIZE = "batch-size";
const char* const MEASUREMENT_BATCH_SIZE_DESIRED = "batch-size-desired";
const char* const MEASUREMENT_VALUES = "values";
const char* const RESULT = "result";
const char* const RESULT_IDLE_RATE = "idle-rate";
const char* const RESULT_NON_IDLE_RATE = "non-idle-rate";
const char* const RESULT_RATE = "rate";
const char* const RESULT_VALUES = "values";
const char* const RESULT_PULSE = "pulse";

static const std::map<Topic, std::pair<const char*, const char*>> TOPIC_MAP{
    { Topic::BatchSize, { MEASUREMENT, MEASUREMENT_BATCH_SIZE }},
    { Topic::BatchSizeDesired, { MEASUREMENT, MEASUREMENT_BATCH_SIZE_DESIRED }},
    { Topic::Measurement, { MEASUREMENT, MEASUREMENT_VALUES }},
    { Topic::Rate, {RESULT, RESULT_RATE }},
    { Topic::Result, {RESULT, RESULT_VALUES }},
    { Topic::IdleRate, {RESULT, RESULT_IDLE_RATE }},
    { Topic::NonIdleRate, {RESULT, RESULT_NON_IDLE_RATE }},
    { Topic::Peak, {RESULT, RESULT_PULSE}},
    { Topic::FreeHeap, {DEVICE, DEVICE_FREE_HEAP }},
    { Topic::FreeStack, {DEVICE, DEVICE_FREE_STACK }},
    { Topic::Error, {DEVICE, DEVICE_ERROR }},
    { Topic::Info, {DEVICE, DEVICE_INFO }},
};

class MqttGateway : public EventClient {
public:
    explicit MqttGateway(EventServer* eventServer);
    // initMqtt is intended for testing purposes, to force not connecting
    void begin(Client* client, const char* clientName, bool initMqtt = true);
    bool connect(const char* user = CONFIG_MQTT_USER, const char* password = CONFIG_MQTT_PASSWORD);
    void publishError(const char* message);

    void handleQueue();
    bool initializeMqtt();
    void update(Topic topic, const char* payload) override;

protected:
    BinaryStatusPublisher _connectionStatus;
    const char* _clientName = 0;
    unsigned long _reconnectTimestamp = 0UL;
    static const int TOPIC_BUFFER_SIZE = 255;
    char _topicBuffer[TOPIC_BUFFER_SIZE] = { 0 };

    bool announceDevice();
    void announceNode(const char* baseTopic, const char* name, const char* type, const char* properties);
    void announceProperty(const char* baseTopic, const char* name, const char* dataType, const char* format, bool settable);
    void callback(const char* topic, byte* payload, unsigned int length);
    bool ensureConnection();
    bool publishEntity(const char* baseTopic, const char* entity, const char* payload);
    bool publishProperty(const char* node, const char* property, const char* payload);
    void subscribeToEventServer();
};

#endif
