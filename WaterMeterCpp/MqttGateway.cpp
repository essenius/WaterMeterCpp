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

#include <cstring>

#include "MqttGateway.h"

using namespace std::placeholders;

constexpr const char* const RATE_RANGE = "0:8640000";
constexpr const char* const TYPE_INTEGER = "integer";
constexpr const char* const TYPE_STRING = "string";
constexpr const char* const LAST_WILL_MESSAGE = "lost";
constexpr bool SETTABLE = true;

// TODO: make property (and potentially inject via constructor)

PubSubClient mqttClient;

constexpr const char* const BASE_TOPIC_TEMPLATE = "homie/%s/%s";

MqttGateway::MqttGateway(
    EventServer* eventServer,
    const char* broker, const int port, const char* user, const char* password,
    const char* buildVersion) :
    EventClient("MqttGateway", eventServer),
    _broker(broker), _user(user), _password(password), _port(port), _buildVersion(buildVersion) {}

void MqttGateway::announceReady() {
    // this is safe to do more than once. So after a disconnect it doesn't hurt
    _eventServer->subscribe(this, Topic::BatchSize); // long
    _eventServer->subscribe(this, Topic::BatchSizeDesired); // long
    _eventServer->subscribe(this, Topic::FreeHeap); // long
    _eventServer->subscribe(this, Topic::FreeStack); // long
    _eventServer->subscribe(this, Topic::Flatline); // long
    _eventServer->subscribe(this, Topic::IdleRate); // long
    _eventServer->subscribe(this, Topic::Measurement); // string
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
    mqttClient.setClient(*client);
    mqttClient.setBufferSize(512);
    mqttClient.setServer(_broker, _port);
    // TODO: optimize in a way that works on Arduino
    mqttClient.setCallback(std::bind(&MqttGateway::callback, this, _1, _2, _3));
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
    sprintf(_topicBuffer, BASE_TOPIC_TEMPLATE, _clientName, "$state");

    bool success;
    if (_user == nullptr || strlen(_user) == 0) {
        success = mqttClient.connect(_clientName, _topicBuffer, 0, true, LAST_WILL_MESSAGE);
    }
    else {
        success = mqttClient.connect(_clientName, _user, _password, _topicBuffer, 0, true, LAST_WILL_MESSAGE);
    }

    // shoudl get picked up by isConnected later
    if (!success) {
        return;
    }

    // if this doesn't work but the connection is still up, we may still be able to run.
    sprintf(_topicBuffer, BASE_TOPIC_TEMPLATE, _clientName, "+/+/set");
    if (!mqttClient.subscribe(_topicBuffer)) {
        publishError("Could not subscribe to setters");
    }
}

void MqttGateway::handleQueue() {
    if (isConnected()) {
        mqttClient.loop();
    }
}

bool MqttGateway::isConnected() {
    return mqttClient.connected();
}

bool MqttGateway::hasAnnouncement() {
    return strlen(_announcementPointer) != 0;
}

void MqttGateway::prepareAnnouncementBuffer() {
    char payload[TOPIC_BUFFER_SIZE];

    prepareEntity("$homie", "4.0.0");
    prepareEntity("$state", "init");
    prepareEntity("$name", _clientName);
    sprintf(payload, "%s,%s,%s", MEASUREMENT, RESULT, DEVICE);
    prepareEntity("$nodes", payload);
    prepareEntity("$implementation", "esp32");
    prepareEntity("$extensions", EMPTY);

    sprintf(payload, "%s,%s,%s", MEASUREMENT_BATCH_SIZE, MEASUREMENT_BATCH_SIZE_DESIRED, MEASUREMENT_VALUES);
    prepareNode(MEASUREMENT, "Measurement", "1", payload);
    prepareProperty(MEASUREMENT, MEASUREMENT_BATCH_SIZE, "Batch Size", TYPE_INTEGER);
    prepareProperty(MEASUREMENT, MEASUREMENT_VALUES, "Values", TYPE_STRING);
    prepareProperty(MEASUREMENT, MEASUREMENT_BATCH_SIZE_DESIRED, "Desired Batch Size", TYPE_INTEGER, "0-20", SETTABLE);

    sprintf(payload, "%s,%s,%s,%s", RESULT_RATE, RESULT_IDLE_RATE, RESULT_NON_IDLE_RATE, RESULT_VALUES);
    prepareNode(RESULT, "Result", "1", payload);
    prepareProperty(RESULT, RESULT_RATE, "Rate", TYPE_INTEGER);
    prepareProperty(RESULT, RESULT_IDLE_RATE, "Idle Rate", TYPE_INTEGER, RATE_RANGE, SETTABLE);
    prepareProperty(RESULT, RESULT_NON_IDLE_RATE, "Non-Idle Rate", TYPE_INTEGER, RATE_RANGE, SETTABLE);
    prepareProperty(RESULT, RESULT_VALUES, "Values", TYPE_STRING);

    sprintf(payload, "%s,%s,%s,%s,%s,%s", DEVICE_FREE_HEAP, DEVICE_FREE_STACK, DEVICE_ERROR, DEVICE_INFO, DEVICE_BUILD,
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
    prepareEntity("$state", "ready");
}

void MqttGateway::prepareEntity(const char* entity, const char* payload) {
    prepareItem(entity);
    prepareItem(payload);
}

void MqttGateway::prepareEntity(const char* baseTopic, const char* entity, const char* payload) {
    sprintf(_announcementPointer, "%s/%s", baseTopic, entity);
    _announcementPointer += strlen(_announcementPointer) + 1;
    prepareItem(payload);
}

void MqttGateway::prepareItem(const char* item) {
    strcpy(_announcementPointer, item);
    _announcementPointer += strlen(_announcementPointer) + 1;
}

void MqttGateway::prepareNode(const char* node, const char* name, const char* type, const char* properties) {
    prepareEntity(node, "$name", name);
    prepareEntity(node, "$type", type);
    prepareEntity(node, "$properties", properties);
}

void MqttGateway::prepareProperty(
    const char* node, const char* property, const char* attribute, const char* dataType, const char* format, const bool settable) {
    sprintf(_topicBuffer, "%s/%s", node, property);
    prepareEntity(_topicBuffer, "$name", attribute);
    prepareEntity(_topicBuffer, "$dataType", dataType);
    if (strlen(format) > 0) {
        prepareEntity(_topicBuffer, "$format", format);
    }
    if (settable) {
        prepareEntity(_topicBuffer, "$settable", "true");
    }
}

bool MqttGateway::publishEntity(const char* baseTopic, const char* entity, const char* payload, bool retain) {
    sprintf(_topicBuffer, BASE_TOPIC_TEMPLATE, baseTopic, entity);
    return mqttClient.publish(_topicBuffer, payload, retain);
}

void MqttGateway::publishError(const char* message) {
    sprintf(_topicBuffer, "MQTT: %s [state = %d]", message, mqttClient.state());
    _eventServer->publish<const char*>(Topic::Error, _topicBuffer);
}

bool MqttGateway::publishNextAnnouncement() {
    const char* topic = _announcementPointer;
    _announcementPointer += strlen(topic) + 1;
    if (strlen(topic) == 0) return false;
    const char* payload = _announcementPointer;
    _announcementPointer += strlen(payload) + 1;
    sprintf(_topicBuffer, BASE_TOPIC_TEMPLATE, _clientName, topic);
    return mqttClient.publish(_topicBuffer, payload, true);
}

bool MqttGateway::publishProperty(const char* node, const char* property, const char* payload, bool retain) {
    char baseTopic[50];
    sprintf(baseTopic, "%s/%s", _clientName, node);
    return publishEntity(baseTopic, property, payload, retain);
}

// incoming event from EventServer
// TODO: this gets lost when we get this during a disconnect. Fix that.
void MqttGateway::update(const Topic topic, const char* payload) {
    const auto entry = TOPIC_MAP.find(topic);
    if (entry != TOPIC_MAP.end()) {
        const auto topicPair = entry->second;
        publishProperty(topicPair.first, topicPair.second, payload);
    }
}
