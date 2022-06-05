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

// following homie convention, see https://homieiot.github.io/specification

#include <ESP.h>
#include <PubSubClient.h>

#include "SafeCString.h"
#include "MqttGateway.h"

using namespace std::placeholders;

// mapping between topics and whether it can be set, the node, and the property
// implementing triplet via two pairs
static const std::map<Topic, std::pair<bool, std::pair<const char*, const char*>>> TOPIC_MAP{
    {Topic::BatchSize,        {false, {MEASUREMENT, MEASUREMENT_BATCH_SIZE}}},
    {Topic::BatchSizeDesired, {true,  {MEASUREMENT, MEASUREMENT_BATCH_SIZE_DESIRED}}},
    {Topic::SamplesFormatted, {false, {MEASUREMENT, MEASUREMENT_VALUES}}},
    {Topic::Rate,             {false, {RESULT, RESULT_RATE}}},
    {Topic::ResultFormatted,  {false, {RESULT, RESULT_VALUES}}},
    {Topic::IdleRate,         {true,  {RESULT, RESULT_IDLE_RATE}}},
    {Topic::NonIdleRate,      {true,  {RESULT, RESULT_NON_IDLE_RATE}}},
    {Topic::Volume,            {false, {RESULT, RESULT_METER}}},
    {Topic::SetVolume,         {true,  {RESULT, RESULT_METER}}},
    {Topic::FreeHeap,         {false, {DEVICE, DEVICE_FREE_HEAP}}},
    {Topic::FreeStack,        {false, {DEVICE, DEVICE_FREE_STACK}}},
    {Topic::FreeQueueSize,    {false, {DEVICE, DEVICE_FREE_QUEUE_SIZE}}},
    {Topic::FreeQueueSpaces,  {false, {DEVICE, DEVICE_FREE_QUEUE_SPACES}}},
    {Topic::SensorWasReset,   {false, {DEVICE, DEVICE_RESET_SENSOR}}},
    {Topic::ResetSensor,      {true,  {DEVICE, DEVICE_RESET_SENSOR}}}
};

static const std::set<Topic> NON_RETAINED_TOPICS{ Topic::ResetSensor, Topic::SensorWasReset, Topic::SetVolume };

static const std::set<Topic> RETRIEVED_TOPICS{ Topic::Volume };


constexpr const char* const RATE_RANGE = "0:8640000";
constexpr const char* const TYPE_INTEGER = "integer";
constexpr const char* const TYPE_STRING = "string";
constexpr const char* const TYPE_FLOAT = "float";
constexpr const char* const LAST_WILL_MESSAGE = "lost";
constexpr bool SETTABLE = true;
constexpr const char* const NAME = "$name";

constexpr const char* const BASE_TOPIC_TEMPLATE = "homie/%s/%s";

MqttGateway::MqttGateway(
    EventServer* eventServer,
    PubSubClient* mqttClient,
    WiFiClientFactory* wifiClientFactory,
    const MqttConfig* mqttConfig,
    const DataQueue* dataQueue,
    const char* buildVersion) :

    EventClient(eventServer),
    _mqttClient(mqttClient),
    _wifiClientFactory(wifiClientFactory),
    _mqttConfig(mqttConfig),
    _dataQueue(dataQueue),
    _buildVersion(buildVersion) {
}

MqttGateway::~MqttGateway() {
    delete _wifiClient;
}

void MqttGateway::announceReady() {
    // this is safe to do more than once. So after a disconnect it doesn't hurt
    // TODO: it's probably OK to do this just once and leave on. Validate.
    _eventServer->subscribe(this, Topic::Alert); 
    _eventServer->subscribe(this, Topic::BatchSize); 
    _eventServer->subscribe(this, Topic::BatchSizeDesired); 
    _eventServer->subscribe(this, Topic::FreeHeap); 
    _eventServer->subscribe(this, Topic::FreeStack); 
    _eventServer->subscribe(this, Topic::FreeQueueSize); 
    _eventServer->subscribe(this, Topic::FreeQueueSpaces); 
    _eventServer->subscribe(this, Topic::IdleRate);
    _eventServer->subscribe(this, Topic::NonIdleRate); 
    _eventServer->subscribe(this, Topic::Rate); 
    _eventServer->subscribe(this, Topic::ResultFormatted); 
    _eventServer->subscribe(this, Topic::SamplesFormatted); // string
    _eventServer->subscribe(this, Topic::SensorWasReset);      
    _eventServer->subscribe(this, Topic::Volume); // string
}

void MqttGateway::begin(const char* clientName) {
    // only do this if it hasn't been done before.
    if (strlen(_announcementBuffer) == 0) {
        _clientName = clientName;
        _announcementPointer = _announcementBuffer;
        prepareAnnouncementBuffer();
    }
    delete _wifiClient;
    _wifiClient = _wifiClientFactory->create(_mqttConfig->useTls);
    _mqttClient->setClient(*_wifiClient);
    _mqttClient->setBufferSize(512);
    _mqttClient->setServer(_mqttConfig->broker, static_cast<uint16_t>(_mqttConfig->port));
    _mqttClient->setCallback([=](const char* topic, const uint8_t* payload, const unsigned int length) {
          this->callback(topic, payload, length);
         });
    connect();
}

// thread safe alternative for strtok
char* nextToken(char** start, const int delimiter) {
    char* token = *start;
    char* nextDelimiter = token != nullptr ? strchr(token, delimiter) : nullptr;
    if (nextDelimiter == nullptr) {
        *start = nullptr;
    }
    else {
        *nextDelimiter = '\0';
        *start = nextDelimiter + 1;
    }
    return token;
}

void MqttGateway::callback(const char* topic, const byte* payload, const unsigned length) {
    char* copyTopic = strdup(topic);
    constexpr int DELIMITER = '/';
    // if the topic is invalid, ignore the message
    if (nextToken(&copyTopic, DELIMITER) == nullptr) return; // homie, ignore
    if (nextToken(&copyTopic, DELIMITER) == nullptr) return; // device ID, ignore
    const char* node = nextToken(&copyTopic, DELIMITER);
    const char* property = nextToken(&copyTopic, DELIMITER);
    const char* set = nextToken(&copyTopic, DELIMITER);
    if (set != nullptr && strcmp(set, "set") == 0) {
        // 1LL is a trick to avoid C26451
        const auto payloadStr = new char[1LL + length];
        for (unsigned int i = 0; i < length; i++) {
            payloadStr[i] = static_cast<char>(payload[i]);
        } 
        payloadStr[length] = 0;
        for (const auto& entry : TOPIC_MAP) {
            const auto topicTriplet = entry.second;
            const auto isSetProperty = topicTriplet.first;
            if (isSetProperty) {
                const auto topicPair = topicTriplet.second;
                if (strcmp(topicPair.first, node) == 0 && strcmp(topicPair.second, property) == 0) {
                    // There is one set-value that requires a string, and that needs to be persistent across tasks
                    if (entry.first == Topic::SetVolume) {
                        safeStrcpy(_volume, payloadStr);
                        _eventServer->publish(this, entry.first, _volume);
                    }
                    else {
                        _eventServer->publish(this, entry.first, payloadStr);
                    }
                    break;
                }
            }
        }
        delete[] payloadStr;
    }
    free(copyTopic);
}

void MqttGateway::connect() {
    _announcementPointer = _announcementBuffer;
    safeSprintf(_topicBuffer, BASE_TOPIC_TEMPLATE, _clientName, STATE);
    _mqttClient->setKeepAlive(90);
    bool success;
    if (_mqttConfig->user == nullptr || strlen(_mqttConfig->user) == 0) {
        success = _mqttClient->connect(_clientName, _topicBuffer, 0, true, LAST_WILL_MESSAGE);
    }
    else {
        success = _mqttClient->connect(_clientName, _mqttConfig->user, _mqttConfig->password, _topicBuffer, 0, true,
                                       LAST_WILL_MESSAGE);
    }

    // should get picked up by isConnected later
    if (!success) {
        return;
    }

    // if this doesn't work but the connection is still up, we may still be able to run.
    safeSprintf(_topicBuffer, BASE_TOPIC_TEMPLATE, _clientName, "+/+/set");
    if (!_mqttClient->subscribe(_topicBuffer)) {
        publishError("Could not subscribe to setters");
    }
}

bool MqttGateway::handleQueue() {
    return isConnected() && _mqttClient->loop();
}

bool MqttGateway::hasAnnouncement() {
    return strlen(_announcementPointer) != 0;
}

bool MqttGateway::isConnected() {
    return _mqttClient->connected();
}

void MqttGateway::prepareAnnouncementBuffer() {
    char payload[TOPIC_BUFFER_SIZE];

    prepareEntity("$homie", "4.0.0");
    prepareEntity(STATE, "init");
    prepareEntity(NAME, _clientName);
    safeSprintf(payload, "%s,%s,%s", MEASUREMENT, RESULT, DEVICE);
    prepareEntity("$nodes", payload);
    prepareEntity("$implementation", "esp32");
    prepareEntity("$extensions", EMPTY);

    safeSprintf(payload, "%s,%s,%s", MEASUREMENT_BATCH_SIZE, MEASUREMENT_BATCH_SIZE_DESIRED, MEASUREMENT_VALUES);
    prepareNode(MEASUREMENT, "Measurement", "1", payload);
    prepareProperty(MEASUREMENT, MEASUREMENT_BATCH_SIZE, "Batch Size", TYPE_INTEGER);
    prepareProperty(MEASUREMENT, MEASUREMENT_BATCH_SIZE_DESIRED, "Desired Batch Size", TYPE_INTEGER, "0-20", SETTABLE);
    prepareProperty(MEASUREMENT, MEASUREMENT_VALUES, "Values", TYPE_STRING);

    safeSprintf(payload, "%s,%s,%s,%s,%s", RESULT_RATE, RESULT_IDLE_RATE, RESULT_NON_IDLE_RATE, RESULT_METER, RESULT_VALUES);
    prepareNode(RESULT, "Result", "1", payload);
    prepareProperty(RESULT, RESULT_RATE, "Rate", TYPE_INTEGER);
    prepareProperty(RESULT, RESULT_IDLE_RATE, "Idle Rate", TYPE_INTEGER, RATE_RANGE, SETTABLE);
    prepareProperty(RESULT, RESULT_NON_IDLE_RATE, "Non-Idle Rate", TYPE_INTEGER, RATE_RANGE, SETTABLE);
    prepareProperty(RESULT, RESULT_METER, "Meter value", TYPE_FLOAT, "0-99999.9999999", SETTABLE);
    prepareProperty(RESULT, RESULT_VALUES, "Values", TYPE_STRING);

    safeSprintf(payload, "%s,%s,%s,%s,%s,%s", DEVICE_FREE_HEAP, DEVICE_FREE_STACK, DEVICE_FREE_QUEUE_SIZE, DEVICE_FREE_QUEUE_SPACES, DEVICE_BUILD, DEVICE_MAC);
    prepareNode(DEVICE, "Device", "1", payload);
    prepareProperty(DEVICE, DEVICE_FREE_HEAP, "Free Heap", TYPE_INTEGER);
    prepareProperty(DEVICE, DEVICE_FREE_STACK, "Free Stack", TYPE_INTEGER);
    prepareProperty(DEVICE, DEVICE_FREE_QUEUE_SIZE, "Free Queue Size", TYPE_INTEGER);
    prepareProperty(DEVICE, DEVICE_FREE_QUEUE_SPACES, "Free Queue Spaces", TYPE_INTEGER);
    prepareProperty(DEVICE, DEVICE_BUILD, "Firmware version", TYPE_STRING);
    prepareProperty(DEVICE, DEVICE_MAC, "Mac address", TYPE_STRING);
    prepareProperty(DEVICE, DEVICE_RESET_SENSOR, "Reset Sensor", TYPE_INTEGER, "1", SETTABLE);

    prepareEntity(DEVICE, DEVICE_BUILD, _buildVersion);
    prepareEntity(DEVICE, DEVICE_MAC, _eventServer->request(Topic::MacFormatted, "unknown"));
    prepareEntity(STATE, "ready");
}

void MqttGateway::prepareEntity(const char* entity, const char* payload) {
    prepareItem(entity);
    prepareItem(payload);
}

void MqttGateway::prepareEntity(const char* baseTopic, const char* entity, const char* payload) {
    safePointerSprintf(_announcementPointer, _announcementBuffer, "%s/%s", baseTopic, entity);
    _announcementPointer += strlen(_announcementPointer) + 1;
    prepareItem(payload);
}

void MqttGateway::prepareItem(const char* item) {
    safePointerStrcpy(_announcementPointer, _announcementBuffer, item);
    _announcementPointer += strlen(_announcementPointer) + 1;
}

void MqttGateway::prepareNode(const char* node, const char* name, const char* type, const char* properties) {
    prepareEntity(node, NAME, name);
    prepareEntity(node, "$type", type);
    prepareEntity(node, "$properties", properties);
}

void MqttGateway::prepareProperty(
    const char* node, const char* property, const char* attribute, const char* dataType, const char* format,
    const bool settable) {
    safeSprintf(_topicBuffer, "%s/%s", node, property);
    prepareEntity(_topicBuffer, NAME, attribute);
    prepareEntity(_topicBuffer, "$dataType", dataType);
    if (strlen(format) > 0) {
        prepareEntity(_topicBuffer, "$format", format);
    }
    if (settable) {
        prepareEntity(_topicBuffer, "$settable", "true");
    }
}

bool MqttGateway::publishEntity(const char* baseTopic, const char* entity, const char* payload, const bool retain) {
    safeSprintf(_topicBuffer, BASE_TOPIC_TEMPLATE, baseTopic, entity);
    return _mqttClient->publish(_topicBuffer, payload, retain);
}

void MqttGateway::publishError(const char* message) {
    safeSprintf(_topicBuffer, "MQTT: %s [state = %d]", message, _mqttClient->state());
    _eventServer->publish<const char*>(Topic::ConnectionError, _topicBuffer);
}

bool MqttGateway::publishNextAnnouncement() {
    const char* topic = _announcementPointer;
    _announcementPointer += strlen(topic) + 1;
    if (strlen(topic) == 0) return false;
    const char* payload = _announcementPointer;
    _announcementPointer += strlen(payload) + 1;
    safeSprintf(_topicBuffer, BASE_TOPIC_TEMPLATE, _clientName, topic);
    return _mqttClient->publish(_topicBuffer, payload, true);
}

bool MqttGateway::publishProperty(const char* node, const char* property, const char* payload, const bool retain) {
    char baseTopic[50];
    safeSprintf(baseTopic, "%s/%s", _clientName, node);
    return publishEntity(baseTopic, property, payload, retain);
}

void MqttGateway::publishUpdate(const Topic topic, const char* payload) {
    if (topic == Topic::Alert) {
        publishEntity(_clientName, STATE, "alert");
        return;
    }
    const auto entry = TOPIC_MAP.find(topic);
    if (entry != TOPIC_MAP.end()) {
        const auto topicTriplet = entry->second;
        const auto isSetTopic = topicTriplet.first;
        if (!isSetTopic) {
            const auto topicPair = topicTriplet.second;
            publishProperty(
                topicPair.first, 
                topicPair.second, 
                payload, 
                NON_RETAINED_TOPICS.find(topic) == NON_RETAINED_TOPICS.end());
        }
    }
}

// incoming event from EventServer. This only happens if we are connected
void MqttGateway::update(const Topic topic, const char* payload) {
    publishUpdate(topic, payload);
}
