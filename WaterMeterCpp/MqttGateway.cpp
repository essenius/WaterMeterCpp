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

#ifdef ESP32
#include <ESP.h>
#include <PubSubClient.h>
#else
// ignore use of deprecated strdup 
#define _CRT_NONSTDC_NO_WARNINGS

#include "ArduinoMock.h"
#include "PubSubClientMock.h"
#endif

#include <string.h>

#include "MqttGateway.h"

using namespace std::placeholders;

const char* RATE_RANGE = "0:8640000";
const char* TYPE_INTEGER = "integer";
const char* LAST_WILL_MESSAGE = "disconnected";

PubSubClient mqttClient;

const char* BASE_TOPIC_TEMPLATE = "homie/%s/%s";

MqttGateway::MqttGateway(EventServer* eventServer) : 
    EventClient("MqttGateway", eventServer), 
    _connectionStatus(eventServer, this, Topic::Disconnected, Topic::Connected) {}

bool MqttGateway::announceDevice() {
    char baseTopic[50];
    char payload[255];
    if (!publishEntity(_clientName, "$homie", "4.0.0")) {
        return false;
    }
    publishEntity(_clientName, "$state", "init");

    publishEntity(_clientName, "$name", _clientName);
    sprintf(payload, "%s,%s,%s", MEASUREMENT, RESULT, DEVICE);
    publishEntity(_clientName, "$nodes", payload);
    publishEntity(_clientName, "$implementation", "esp32");
    publishEntity(_clientName, "$extensions", "");
    sprintf(baseTopic, "%s/%s", _clientName, MEASUREMENT);
    sprintf(payload, "%s,%s,%s", MEASUREMENT_BATCH_SIZE, MEASUREMENT_BATCH_SIZE_DESIRED, MEASUREMENT_VALUES);
    announceNode(baseTopic, "Measurement", "1", payload);
    strcat(baseTopic, "/");
    strcat(baseTopic, MEASUREMENT_BATCH_SIZE);
    announceProperty(baseTopic, "Batch Size", TYPE_INTEGER, "", false);
    sprintf(baseTopic, "%s/%s/%s", _clientName, MEASUREMENT, MEASUREMENT_VALUES);
    announceProperty(baseTopic, "Values", "string", "", true);
    sprintf(baseTopic, "%s/%s/%s", _clientName, MEASUREMENT, MEASUREMENT_BATCH_SIZE_DESIRED);
    announceProperty(baseTopic, "Desired Batch Size", TYPE_INTEGER, "0-20", true);
    sprintf(baseTopic, "%s/%s", _clientName, RESULT);
    sprintf(payload, "%s,%s,%s,%s", RESULT_RATE, RESULT_IDLE_RATE, RESULT_NON_IDLE_RATE, RESULT_PULSE);
    announceNode(baseTopic, "Result", "1", payload);
    strcat(baseTopic, "/");
    strcat(baseTopic, RESULT_RATE);
    announceProperty(baseTopic, "Rate", TYPE_INTEGER, "", false);
    sprintf(baseTopic, "%s/%s/%s", _clientName, RESULT, RESULT_IDLE_RATE);
    announceProperty(baseTopic, "Idle Rate", TYPE_INTEGER, RATE_RANGE, true);
    sprintf(baseTopic, "%s/%s/%s", _clientName, RESULT, RESULT_NON_IDLE_RATE);
    announceProperty(baseTopic, "Non-Idle Rate", TYPE_INTEGER, RATE_RANGE, false);
    sprintf(baseTopic, "%s/%s/%s", _clientName, RESULT, RESULT_PULSE);
    announceProperty(baseTopic, "PULSE", TYPE_INTEGER, "", false);
    
    sprintf(baseTopic, "%s/%s", _clientName, DEVICE);
    sprintf(payload, "%s,%s,%s,%s", DEVICE_FREE_HEAP, DEVICE_FREE_STACK, DEVICE_ERROR, DEVICE_INFO);
    announceNode(baseTopic, "Device", "1", payload);
    strcat(baseTopic, "/");
    strcat(baseTopic, DEVICE_FREE_HEAP);
    announceProperty(baseTopic, "Free Heap Memory", "integer", "", false);
    sprintf(baseTopic, "%s/%s/%s", _clientName, DEVICE, DEVICE_FREE_STACK);
    announceProperty(baseTopic, "Free Stack Memory", TYPE_INTEGER, "", false);
    sprintf(baseTopic, "%s/%s/%s", _clientName, DEVICE, DEVICE_ERROR);
    announceProperty(baseTopic, "Error message", "string", "", false);
    sprintf(baseTopic, "%s/%s/%s", _clientName, DEVICE, DEVICE_INFO);
    announceProperty(baseTopic, "Info message", "string", "", false);

    publishEntity(_clientName, "$state", "ready");
    _eventServer->publish(Topic::Info, "MQTT: Announcement complete");
    return true;
}

void MqttGateway::announceNode(const char* baseTopic, const char* name, const char* type, const char* properties) {
    publishEntity(baseTopic, "$name", name);
    publishEntity(baseTopic, "$type", type);
    publishEntity(baseTopic, "$properties", properties);
}

void MqttGateway::announceProperty(const char* baseTopic, const char* name, const char* dataType, const char* format, bool settable) {
    publishEntity(baseTopic, "$name", name);
    publishEntity(baseTopic, "$dataType", dataType);
    if (strlen(format) > 0) {
        publishEntity(baseTopic, "$format", format);
    }
    if (settable) {
        publishEntity(baseTopic, "$settable", "true");
    }
}

void MqttGateway::begin(Client* client, const char* clientName, bool initMqtt) {
    _connectionStatus.setState(false);
    mqttClient.setClient(*client);
    _clientName = clientName;
    mqttClient.setBufferSize(512);
    mqttClient.setServer(CONFIG_MQTT_BROKER, CONFIG_MQTT_PORT);
    mqttClient.setCallback(std::bind(&MqttGateway::callback, this, _1, _2, _3));
    if (initMqtt && initializeMqtt()) {
        _connectionStatus.setState(true);
    }
}

void MqttGateway::callback(const char* topic, byte* payload, unsigned int length) {
    char* copyTopic = strdup(topic);
    // if the topic is invalid, ignore the message
    if (strtok(copyTopic, "/") == NULL) return;  // homie, ignore
    if (strtok(NULL, "/") == NULL) return;      //device ID, ignore
    char* node = strtok(NULL, "/");
    char* property = strtok(NULL, "/");
    char* set = strtok(NULL, "/");
    if (strcmp(set, "set") == 0) {
        // 1ULL is a trick to avoid C26451
        char* payloadStr = new char[1ULL + length];
        for (unsigned int i = 0; i < length; i++) {
            payloadStr[i] = payload[i];
        }
        payloadStr[length] = 0;
        for (auto const& entry : TOPIC_MAP) {
            auto topicPair = entry.second;
            if (strcmp(topicPair.first, node) == 0 && strcmp(topicPair.second, property) == 0) {
                _eventServer->publish<const char*>(this, entry.first, payloadStr);
                break;
            }
        }
        delete[] payloadStr;
    }
    free(copyTopic);
}

bool MqttGateway::connect(const char* user, const char* password) {
    _eventServer->publish(Topic::Info, "MQTT: Connecting");
    char lastWillTopic[TOPIC_BUFFER_SIZE];
    sprintf(lastWillTopic, "%s/%s/%s", _clientName, DEVICE, DEVICE_ERROR);

    if (strlen(user) == 0) {
        _connectionStatus.setState(mqttClient.connect(_clientName, lastWillTopic, 0, false, LAST_WILL_MESSAGE));
    }
    else {
        _connectionStatus.setState(mqttClient.connect(_clientName, CONFIG_MQTT_USER, CONFIG_MQTT_PASSWORD, 
            lastWillTopic, 0, false, LAST_WILL_MESSAGE));
    }

    if (!_connectionStatus.getState()) {
        publishError("Could not connect to broker");
        return false;
    }

    sprintf(_topicBuffer, BASE_TOPIC_TEMPLATE, _clientName, "+/+/set");
    if (!mqttClient.subscribe(_topicBuffer)) {
        publishError("Could not subscribe to setters");
        _connectionStatus.setState(false);
        return false;
    }
    _eventServer->publish(Topic::Info, "MQTT: Connected and subscribed to setters");
    _eventServer->publish(Topic::Error, "");
    return true;
}

/// <summary>Ensure we still have a connection. If not, re-initialize try reconnecting</summary>
/// <returns>whether or not we are connected</returns>
bool MqttGateway::ensureConnection() {
    if (!mqttClient.connected()) _connectionStatus.setState(false);
    if (_connectionStatus.getState()) return true;
    mute(true);

    // don't reconnect more than once every second
    if (_reconnectTimestamp == 0UL || micros() - _reconnectTimestamp > 1000000UL) {
        _reconnectTimestamp = micros();
        if (initializeMqtt()) {
            // TODO: do we still need this? looks a bit kludgy
            _eventServer->publish(Topic::Error, "");
            _reconnectTimestamp = 0;
            return true;
        }
    }
    return false;
}

void MqttGateway::handleQueue() {
    if (ensureConnection()) {
        mqttClient.loop();
    }
}

/// <summary>Connect to the broker, announce the device, and subscribe to the event server</summary>
/// <returns>whether successful</returns>
/// <remarks>
/// Requires: Not already subscribed to event server
/// Guarantees: _connectionStatus is set to the return value
/// </remarks>
bool MqttGateway::initializeMqtt() {

    if (connect()) {
        if (announceDevice()) {
            subscribeToEventServer();
            //_eventServer->publish(Topic::Error, "");
            return true;
        }
        publishError("Could not announce device");
        _connectionStatus.setState(false);
        return false;
    }
    return false;
}

bool MqttGateway::publishEntity(const char* baseTopic, const char* entity, const char* payload) {
    if (!ensureConnection()) return false;
    sprintf(_topicBuffer, BASE_TOPIC_TEMPLATE, baseTopic, entity);
    return _connectionStatus.setState(mqttClient.publish(_topicBuffer, payload));
}

void MqttGateway::publishError(const char* message) {
    sprintf(_topicBuffer, "MQTT: %s [state = %d]", message, mqttClient.state());
    _eventServer->publish<const char*>(Topic::Error, _topicBuffer);
}

bool MqttGateway::publishProperty(const char* node, const char* property, const char* payload) {
    char baseTopic[50];
    sprintf(baseTopic, "%s/%s", _clientName, node);
    return publishEntity(baseTopic, property, payload);
}

void MqttGateway::subscribeToEventServer() {
    // this is safe to do more than once. So after a disconnect it doesn't hurt
    _eventServer->subscribe(this, Topic::Peak);             // long
    _eventServer->subscribe(this, Topic::BatchSize);        // long
    _eventServer->subscribe(this, Topic::BatchSizeDesired); // long
    _eventServer->subscribe(this, Topic::FreeHeap);         // long
    _eventServer->subscribe(this, Topic::FreeStack);        // long
    _eventServer->subscribe(this, Topic::IdleRate);         // long
    _eventServer->subscribe(this, Topic::Measurement);      // string
    _eventServer->subscribe(this, Topic::NonIdleRate);      // long
    _eventServer->subscribe(this, Topic::Rate);             // long  
    _eventServer->subscribe(this, Topic::Result);           // string
    _eventServer->subscribe(this, Topic::Error);            // string 
    _eventServer->subscribe(this, Topic::Info);             // string
    // making sure we are listening. Mute is on after a disconnect.
    mute(false);
}

// incoming event from EventServer
// TODO: this gets lost when we get this during a disconnect. Fix that.
void MqttGateway::update(Topic topic, const char* payload) {
    auto entry = TOPIC_MAP.find(topic);
    if (entry != TOPIC_MAP.end()) {
        auto topicPair = entry->second;
        publishProperty(topicPair.first, topicPair.second, payload);
    }
};

/* handled by EventClient 
void MqttGateway::update(Topic topic, long payload) {
    char numberBuffer[20];
    sprintf(numberBuffer, "%ld", payload);
    update(topic, numberBuffer);
}; */
