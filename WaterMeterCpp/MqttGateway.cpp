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

// following homie convention, see https://homieiot.github.io/specification

#ifdef ESP32
#include <ESP.h>
#include <PubSubClient.h>
#else
#include "ArduinoMock.h"
#include "PubSubClientMock.h"
#endif

#include "SafeCString.h"

#include "MqttGateway.h"

using namespace std::placeholders;

constexpr const char* const RATE_RANGE = "0:8640000";
constexpr const char* const TYPE_INTEGER = "integer";
constexpr const char* const TYPE_STRING = "string";
constexpr const char* const LAST_WILL_MESSAGE = "lost";
constexpr bool SETTABLE = true;
constexpr const char* const NAME = "$name";


constexpr const char* const BASE_TOPIC_TEMPLATE = "homie/%s/%s";

MqttGateway::MqttGateway(
    EventServer* eventServer, PubSubClient* mqttClient, const MqttConfig* mqttConfig, const DataQueue* dataQueue, const char* buildVersion) :
    EventClient(eventServer),
    _mqttClient(mqttClient),
    _mqttConfig(mqttConfig),
    _dataQueue(dataQueue),
    _buildVersion(buildVersion) {}

void MqttGateway::announceReady() {
    // this is safe to do more than once. So after a disconnect it doesn't hurt
    _eventServer->subscribe(this, Topic::Alert); // long
    _eventServer->subscribe(this, Topic::BatchSize); // long
    _eventServer->subscribe(this, Topic::BatchSizeDesired); // long
    _eventServer->subscribe(this, Topic::FreeHeap); // long
    _eventServer->subscribe(this, Topic::FreeStack); // long
    _eventServer->subscribe(this, Topic::IdleRate); // long
    _eventServer->subscribe(this, Topic::Samples); // string
    _eventServer->subscribe(this, Topic::NonIdleRate); // long
    _eventServer->subscribe(this, Topic::Rate); // long  
    _eventServer->subscribe(this, Topic::Result); // string
    _eventServer->subscribe(this, Topic::Error); // string 
    _eventServer->subscribe(this, Topic::Info); // string
    // making sure we are listening. Mute is on after a disconnect.
    mute(false);
}

void MqttGateway::begin(Client* client, const char* clientName) {
    _clientName = clientName;
    _announcementPointer = _announcementBuffer;
    // only do this if it hasn't been done before.
    if (strlen(_announcementBuffer) == 0) {
        prepareAnnouncementBuffer();
    }
    _mqttClient->setClient(*client);
    _mqttClient->setBufferSize(512);
    _mqttClient->setServer(_mqttConfig->broker, _mqttConfig->port);
    // TODO: optimize in a way that works on Arduino
    _mqttClient->setCallback(std::bind(&MqttGateway::callback, this, _1, _2, _3));
    connect();
}

void MqttGateway::callback(const char* topic, const byte* payload, const unsigned length) {
    char* copyTopic = strdup(topic);
    // TODO find alternative for strtok (which is a bit tricky)
    // if the topic is invalid, ignore the message
    if (strtok(copyTopic, "/") == nullptr) return; // homie, ignore
    if (strtok(nullptr, "/") == nullptr) return; //device ID, ignore
    const char* node = strtok(nullptr, "/");
    const char* property = strtok(nullptr, "/");
    const char* set = strtok(nullptr, "/");
    if (strcmp(set, "set") == 0) {
        // 1ULL is a trick to avoid C26451
        const auto payloadStr = new char[1ULL + length];
        for (unsigned int i = 0; i < length; i++) {
            payloadStr[i] = static_cast<char>(payload[i]);
        }
        payloadStr[length] = 0;
        for (const auto& entry : TOPIC_MAP) {
            const auto topicPair = entry.second;
            if (strcmp(topicPair.first, node) == 0 && strcmp(topicPair.second, property) == 0) {
                _eventServer->publish<const char*>(this, entry.first, payloadStr);
                break;
            }
        }
        delete[] payloadStr;
    }
    free(copyTopic);
}

void MqttGateway::connect() {
    _announcementPointer = _announcementBuffer;
    safeSprintf(_topicBuffer, BASE_TOPIC_TEMPLATE, _clientName, STATE);

    bool success;
    if (_mqttConfig->user == nullptr || strlen(_mqttConfig->user) == 0) {
        success = _mqttClient->connect(_clientName, _topicBuffer, 0, true, LAST_WILL_MESSAGE);
    }
    else {
        success = _mqttClient->connect(_clientName, _mqttConfig->user, _mqttConfig->password, _topicBuffer, 0, true, LAST_WILL_MESSAGE);
    }

    // shoudl get picked up by isConnected later
    if (!success) {
        return;
    }

    // if this doesn't work but the connection is still up, we may still be able to run.
    safeSprintf(_topicBuffer, BASE_TOPIC_TEMPLATE, _clientName, "+/+/set");
    if (!_mqttClient->subscribe(_topicBuffer)) {
        publishError("Could not subscribe to setters");
    }
}

void MqttGateway::handleQueue() {
    if (isConnected()) {
        _mqttClient->loop();
    }
}

bool MqttGateway::isConnected() {
    return _mqttClient->connected();
}

bool MqttGateway::hasAnnouncement() {
    return strlen(_announcementPointer) != 0;
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
    prepareProperty(MEASUREMENT, MEASUREMENT_VALUES, "Values", TYPE_STRING);
    prepareProperty(MEASUREMENT, MEASUREMENT_BATCH_SIZE_DESIRED, "Desired Batch Size", TYPE_INTEGER, "0-20", SETTABLE);

    safeSprintf(payload, "%s,%s,%s,%s", RESULT_RATE, RESULT_IDLE_RATE, RESULT_NON_IDLE_RATE, RESULT_VALUES);
    prepareNode(RESULT, "Result", "1", payload);
    prepareProperty(RESULT, RESULT_RATE, "Rate", TYPE_INTEGER);
    prepareProperty(RESULT, RESULT_IDLE_RATE, "Idle Rate", TYPE_INTEGER, RATE_RANGE, SETTABLE);
    prepareProperty(RESULT, RESULT_NON_IDLE_RATE, "Non-Idle Rate", TYPE_INTEGER, RATE_RANGE, SETTABLE);
    prepareProperty(RESULT, RESULT_VALUES, "Values", TYPE_STRING);

    safeSprintf(payload, "%s,%s,%s,%s,%s,%s", DEVICE_FREE_HEAP, DEVICE_FREE_STACK, DEVICE_ERROR, DEVICE_INFO,
                DEVICE_BUILD,
                DEVICE_MAC);
    prepareNode(DEVICE, "Device", "1", payload);
    prepareProperty(DEVICE, DEVICE_FREE_HEAP, "Free Heap Memory", TYPE_INTEGER);
    prepareProperty(DEVICE, DEVICE_FREE_STACK, "Free Stack Memory", TYPE_INTEGER);
    prepareProperty(DEVICE, DEVICE_ERROR, "Error message", TYPE_STRING);
    prepareProperty(DEVICE, DEVICE_INFO, "Info message", TYPE_STRING);
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
    _eventServer->publish<const char*>(Topic::Error, _topicBuffer);
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
        const auto topicPair = entry->second;
        publishProperty(topicPair.first, topicPair.second, payload);
    }
}

void MqttGateway::storePendingMessage(const Topic topic, const long payload) {
    _pendingMessages[topic] = payload;
}

void MqttGateway::sendPendingMessages() {
    for (auto iterator = _pendingMessages.begin(); iterator != _pendingMessages.end();) {
        update(iterator->first, iterator->second);
        iterator = _pendingMessages.erase(iterator);
    }
}

void MqttGateway::update(const Topic topic, long payload) {
    if (!isConnected()) {
        storePendingMessage(topic, payload);
        return;
    }
    // making sure we don't call update (topic, const char*) here
    char numberBuffer[20];
    safeSprintf(numberBuffer, "%ld", payload);
    publishUpdate(topic, numberBuffer);
}

// incoming event from EventServer
void MqttGateway::update(const Topic topic, const char* payload) {
    if (!isConnected()) {
        if (topic == Topic::Error || topic == Topic::Info) {
            RingbufferPayload bufferPayload{};
            bufferPayload.topic = topic;
            bufferPayload.timestamp = Clock::getTimestamp();
            safeStrcpy(bufferPayload.buffer.message, payload);
            // if this doesn't work (e.g. buffer full), we discard the message
            if (!_dataQueue->send(&bufferPayload)) {
                _eventServer->publish(Topic::Blocked, LONG_TRUE);
            }
        }
        // discard other topics. Results and samples should not come in while disconnected
        return;
    }
    publishUpdate(topic, payload);
}
