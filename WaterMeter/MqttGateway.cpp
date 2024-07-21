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

// following homie convention, see https://homieiot.github.io/specification

#include <ESP.h>
#include <PubSubClient.h>

#include <SafeCString.h>
#include "MqttGateway.h"

#include <memory>

namespace WaterMeter {
    using namespace std::placeholders;

    // mapping between topics and whether it can be set, the node, and the property
    // implementing triplet via two pairs
    static const std::map<Topic, std::pair<bool, std::pair<const char*, const char*>>> TopicMap{
        {Topic::BatchSize, {false, {Measurement, MeasurementBatchSize}}},
        {Topic::BatchSizeDesired, {true, {Measurement, MeasurementBatchSizeDesired}}},
        {Topic::SamplesFormatted, {false, {Measurement, MeasurementValues}}},
        {Topic::Rate, {false, {Result, ResultRate}}},
        {Topic::ResultFormatted, {false, {Result, ResultValues}}},
        {Topic::IdleRate, {true, {Result, ResultIdleRate}}},
        {Topic::NonIdleRate, {true, {Result, ResultNonIdleRate}}},
        {Topic::MeterPayload, {false, {Result, ResultMeter}}},
        {Topic::SetVolume, {true, {Result, ResultMeter}}},
        {Topic::FreeHeap, {false, {DeviceLabel, DeviceFreeHeap}}},
        {Topic::FreeStack, {false, {DeviceLabel, DeviceFreeStack}}},
        {Topic::FreeQueueSize, {false, {DeviceLabel, DeviceFreeQueueSize}}},
        {Topic::FreeQueueSpaces, {false, {DeviceLabel, DeviceFreeQueueSpaces}}},
        {Topic::SensorWasReset, {false, {DeviceLabel, DeviceResetSensor}}},
        {Topic::ResetSensor, {true, {DeviceLabel, DeviceResetSensor}}}
    };

    static const std::set<Topic> NonRetainedTopics{ Topic::ResetSensor, Topic::SensorWasReset };

    constexpr auto RateRange = "0:8640000";
    constexpr auto TypeInteger = "integer";
    constexpr auto TypeString = "string";
    constexpr auto LastWillMessage = "lost";
    constexpr bool Settable = true;
    constexpr auto Name = "$name";

    constexpr auto BaseTopicTemplate = "homie/%s/%s";

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
        _buildVersion(buildVersion) {}

    MqttGateway::~MqttGateway() {
        delete _wifiClient;
    }

    // ---- Public methods ----

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
        _eventServer->subscribe(this, Topic::MeterPayload); // string
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
        _mqttClient->setCallback([this](const char* topic, const uint8_t* payload, const unsigned int length) {
            this->callback(topic, payload, length);
            });
        connect();
    }

    void MqttGateway::connect() {
        _announcementPointer = _announcementBuffer;
        SafeCString::sprintf(_topicBuffer, BaseTopicTemplate, _clientName, State);
        _mqttClient->setKeepAlive(90);
        bool success;
        if (_mqttConfig->user == nullptr || strlen(_mqttConfig->user) == 0) {
            success = _mqttClient->connect(_clientName, _topicBuffer, 0, true, LastWillMessage);
        }
        else {
            success = _mqttClient->connect(_clientName, _mqttConfig->user, _mqttConfig->password, _topicBuffer, 0, true,
                LastWillMessage);
        }

        // should get picked up by isConnected later
        if (!success) {
            return;
        }

        // if this doesn't work but the connection is still up, we may still be able to run.
        SafeCString::sprintf(_topicBuffer, BaseTopicTemplate, _clientName, "+/+/set");
        if (!_mqttClient->subscribe(_topicBuffer)) {
            publishError("Could not subscribe to setters");
        }

        if (!_justStarted) return;

        // just after a boot, see if we have a previous meter value

        SafeCString::sprintf(_topicBuffer, BaseTopicTemplate, _clientName, Result);
        SafeCString::strcat(_topicBuffer, "/");
        SafeCString::strcat(_topicBuffer, ResultMeter);

        if (!_mqttClient->subscribe(_topicBuffer)) {
            publishError("Could not subscribe to result meter");
        }
    }

    bool MqttGateway::getPreviousVolume() {
        if (!_justStarted) return false;
        const auto startTime = micros();
        while (!_meterPayloadReceived && micros() - startTime < 1e6) {
            _mqttClient->loop();
            delay(10);
        }
        _justStarted = false;
        _mqttClient->unsubscribe(_topicBuffer);
        return true;
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

    bool MqttGateway::publishNextAnnouncement() {
        const char* topic = _announcementPointer;
        _announcementPointer += strlen(topic) + 1;
        if (strlen(topic) == 0) return false;
        const char* payload = _announcementPointer;
        _announcementPointer += strlen(payload) + 1;
        SafeCString::sprintf(_topicBuffer, BaseTopicTemplate, _clientName, topic);
        return _mqttClient->publish(_topicBuffer, payload, true);
    }


    // incoming event from EventServer. This only happens if we are connected
    void MqttGateway::update(const Topic topic, const char* payload) {
        publishToMqtt(topic, payload);
    }

    // ---- Protected methods ----

    void MqttGateway::callback(const char* topic, const byte* payload, const unsigned length) {
        // ignore messages with an empty payload.
        if (length == 0) return;

        auto deleter = [](char* ptr) { free(ptr); };
        std::unique_ptr<char, decltype(deleter)> copyTopicPointer(_strdup(topic), deleter);
        char* copyTopic = copyTopicPointer.get();
        constexpr int Delimiter = '/';
        // if the topic is invalid, ignore the message
        if (nextToken(&copyTopic, Delimiter) == nullptr) return; // homie, ignore
        if (nextToken(&copyTopic, Delimiter) == nullptr) return; // device ID, ignore
        const char* node = nextToken(&copyTopic, Delimiter);
        const char* property = nextToken(&copyTopic, Delimiter);
        const char* set = nextToken(&copyTopic, Delimiter);
        if (node == nullptr || property == nullptr) return;
        const auto isSetter = set != nullptr && strcmp(set, "set") == 0;
        const auto payloadStr = std::unique_ptr<char[]>(new char[length + 1]);
        for (unsigned int i = 0; i < length; i++) {
            payloadStr[i] = static_cast<char>(payload[i]);
        }
        payloadStr[length] = 0;
        for (const auto& entry : TopicMap) {
            const auto topicTriplet = entry.second;
            const auto isSetProperty = topicTriplet.first;
            if (isSetter == isSetProperty) {
                const auto topicPair = topicTriplet.second;
                if (isRightTopic(topicPair, node, property)) {
                    publishToEventServer(entry.first, payloadStr.get());
                    break;
                }
            }
        }
    }

    bool MqttGateway::isRightTopic(const std::pair<const char*, const char*> topicPair, const char* expectedNode, const char* expectedProperty) {
        return strcmp(topicPair.first, expectedNode) == 0 && strcmp(topicPair.second, expectedProperty) == 0;
    }

    void MqttGateway::prepareAnnouncementBuffer() {
        char payload[TopicBufferSize];

        prepareEntity("$homie", "4.0.0");
        prepareEntity(State, "init");
        prepareEntity(Name, _clientName);
        SafeCString::sprintf(payload, "%s,%s,%s", Measurement, Result, DeviceLabel);
        prepareEntity("$nodes", payload);
        prepareEntity("$implementation", "esp32");
        prepareEntity("$extensions", Empty);

        SafeCString::sprintf(payload, "%s,%s,%s", MeasurementBatchSize, MeasurementBatchSizeDesired, MeasurementValues);
        prepareNode(Measurement, "Measurement", "1", payload);
        prepareProperty(Measurement, MeasurementBatchSize, "Batch Size", TypeInteger);
        prepareProperty(Measurement, MeasurementBatchSizeDesired, "Desired Batch Size", TypeInteger, "0-20", Settable);
        prepareProperty(Measurement, MeasurementValues, "Values", TypeString);

        SafeCString::sprintf(payload, "%s,%s,%s,%s,%s", ResultRate, ResultIdleRate, ResultNonIdleRate, ResultMeter, ResultValues);
        prepareNode(Result, "Result", "1", payload);
        prepareProperty(Result, ResultRate, "Rate", TypeInteger);
        prepareProperty(Result, ResultIdleRate, "Idle Rate", TypeInteger, RateRange, Settable);
        prepareProperty(Result, ResultNonIdleRate, "Non-Idle Rate", TypeInteger, RateRange, Settable);
        prepareProperty(Result, ResultMeter, "Meter value", TypeString, Empty, Settable); // exception, settable via different topic
        prepareProperty(Result, ResultValues, "Values", TypeString);

        SafeCString::sprintf(payload, "%s,%s,%s,%s,%s,%s", DeviceFreeHeap, DeviceFreeStack, DeviceFreeQueueSize,
            DeviceFreeQueueSpaces, DeviceBuild, DeviceMac);
        prepareNode(DeviceLabel, "DeviceLabel", "1", payload);
        prepareProperty(DeviceLabel, DeviceFreeHeap, "Free Heap", TypeInteger);
        prepareProperty(DeviceLabel, DeviceFreeStack, "Free Stack", TypeInteger);
        prepareProperty(DeviceLabel, DeviceFreeQueueSize, "Free Queue Size", TypeInteger);
        prepareProperty(DeviceLabel, DeviceFreeQueueSpaces, "Free Queue Spaces", TypeInteger);
        prepareProperty(DeviceLabel, DeviceBuild, "Firmware version", TypeString);
        prepareProperty(DeviceLabel, DeviceMac, "Mac address", TypeString);
        prepareProperty(DeviceLabel, DeviceResetSensor, "Reset Sensor", TypeInteger, "1", Settable);

        prepareEntity(DeviceLabel, DeviceBuild, _buildVersion);
        prepareEntity(DeviceLabel, DeviceMac, _eventServer->request(Topic::MacFormatted, "unknown"));
        prepareEntity(State, "ready");
    }

    void MqttGateway::prepareEntity(const char* entity, const char* payload) {
        prepareItem(entity);
        prepareItem(payload);
    }

    void MqttGateway::prepareEntity(const char* baseTopic, const char* entity, const char* payload) {
        SafeCString::pointerSprintf(_announcementPointer, _announcementBuffer, "%s/%s", baseTopic, entity);
        _announcementPointer += strlen(_announcementPointer) + 1;
        prepareItem(payload);
    }

    void MqttGateway::prepareItem(const char* item) {
        SafeCString::pointerStrcpy(_announcementPointer, _announcementBuffer, item);
        _announcementPointer += strlen(_announcementPointer) + 1;
    }

    void MqttGateway::prepareNode(const char* node, const char* name, const char* type, const char* properties) {
        prepareEntity(node, Name, name);
        prepareEntity(node, "$type", type);
        prepareEntity(node, "$properties", properties);
    }

    void MqttGateway::prepareProperty(
        const char* node, const char* property, const char* attribute, const char* dataType, const char* format,
        const bool settable) {
        SafeCString::sprintf(_topicBuffer, "%s/%s", node, property);
        prepareEntity(_topicBuffer, Name, attribute);
        prepareEntity(_topicBuffer, "$dataType", dataType);
        if (strlen(format) > 0) {
            prepareEntity(_topicBuffer, "$format", format);
        }
        if (settable) {
            prepareEntity(_topicBuffer, "$settable", "true");
        }
    }

    bool MqttGateway::publishEntity(const char* baseTopic, const char* entity, const char* payload, const bool retain) {
        SafeCString::sprintf(_topicBuffer, BaseTopicTemplate, baseTopic, entity);
        return _mqttClient->publish(_topicBuffer, payload, retain);
    }

    void MqttGateway::publishError(const char* message) {
        SafeCString::sprintf(_topicBuffer, "MQTT: %s [state = %d]", message, _mqttClient->state());
        _eventServer->publish<const char*>(Topic::ConnectionError, _topicBuffer);
    }

    bool MqttGateway::publishProperty(const char* node, const char* property, const char* payload, const bool retain) {
        char baseTopic[50];
        SafeCString::sprintf(baseTopic, "%s/%s", _clientName, node);
        return publishEntity(baseTopic, property, payload, retain);
    }

    void MqttGateway::publishToEventServer(const Topic topic, const char* payload) {
        if (topic != Topic::SetVolume && topic != Topic::MeterPayload) {
            _eventServer->publish(this, topic, payload);
            return;
        }
        _meterPayloadReceived = true;
        // There is one set-value that requires a string, and that needs to be persistent across tasks
        // This also happens to be the only value that can be set from the getter (just once, right after reboot)
        SafeCString::strcpy(_meterPayload, payload);
        const auto topicToSend = topic == Topic::SetVolume ? Topic::SetVolume : Topic::AddVolume;
        _eventServer->publish(this, topicToSend, _meterPayload);
    }

    void MqttGateway::publishToMqtt(const Topic topic, const char* payload) {
        if (topic == Topic::Alert) {
            publishEntity(_clientName, State, "alert");
            return;
        }
        const auto entry = TopicMap.find(topic);
        if (entry != TopicMap.end()) {
            const auto topicTriplet = entry->second;
            const auto isSetTopic = topicTriplet.first;
            if (!isSetTopic) {
                const auto topicPair = topicTriplet.second;
                publishProperty(
                    topicPair.first,
                    topicPair.second,
                    payload,
                    NonRetainedTopics.find(topic) == NonRetainedTopics.end());
            }
        }
    }
}
