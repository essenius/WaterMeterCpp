// Copyright 2022 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#include "QueueClient.h"
#include "EventServer.h"

struct ShortMessage {
    // we're using a trick here. The most significant bit is used to codify whether we have a char* (1) or a long (0)
    int16_t topic;
    intptr_t payload;
};

QueueHandle_t QueueClient::createQueue(const uint16_t length) {
    if (length == 0) return nullptr;
    return xQueueCreate(length, sizeof(ShortMessage));
}

QueueClient::QueueClient(EventServer* eventServer, Log* logger, const uint16_t size, const int8_t index) :
    EventClient(eventServer),
    _logger(logger),
    _freeSpaces(eventServer, Topic::FreeQueueSpaces, 5, 0, index, size),
    // being careful with reporting on spaces as it may use them as well, so just every 5
    _receiveQueue(createQueue(size)),
    _index(index) {
}

// ReSharper disable once CppParameterMayBeConst -- introduces misplaced const
void QueueClient::begin(QueueHandle_t sendQueue) {
    _sendQueue = sendQueue;
}

QueueHandle_t QueueClient::getQueueHandle() const {
    return _receiveQueue;
}

bool QueueClient::receive() {
    if (_receiveQueue == nullptr || uxQueueMessagesWaiting(_receiveQueue) == 0) return false;
    ShortMessage message{};
    if (xQueueReceive(_receiveQueue, &message, 0) == pdFALSE) return false;
    if (message.topic < 0) {
        _eventServer->publish(
            this, 
            static_cast<Topic>(message.topic & 0x7fff), 
            reinterpret_cast<const char*>(message.payload));
    }
    else {
        _eventServer->publish(this, static_cast<Topic>(message.topic), static_cast<long>(message.payload));
    }
    // conversion should not be a problem - values don't get large
    _freeSpaces = static_cast<long>(uxQueueSpacesAvailable(_receiveQueue));
    return true;
}

void QueueClient::update(const Topic topic, const char* payload) {
    // if we can convert the input to a long, do that. Otherwise keep it a string
    char* endPointer;
    const auto longValue = strtol(payload, &endPointer, 0);
    if (*endPointer != '\0') {
        send(topic, reinterpret_cast<intptr_t>(payload), true);
        return;
    } 
    send(topic, longValue, false);
}

void QueueClient::update(const Topic topic, const long payload) {
    send(topic, payload, false);
}

void QueueClient::send(const Topic topic, const intptr_t payload, const bool isString) {
    if (_sendQueue == nullptr) return;
    auto topic1 = static_cast<int16_t>(topic);
    if (isString) topic1 |= static_cast<int16_t>(0x8000);
    const ShortMessage message = { topic1, payload };
    if (xQueueSendToBack(_sendQueue, &message, 0) == pdFALSE) {
        // Catch 22 - we may need a queue to send an error, and that fails. So we're using a direct log.
        // That uses the default format which gives more details 
        _logger->log("[E] Instance %p (%d): error sending %d/%lld\n", this, _index, topic, payload);
    }
}
