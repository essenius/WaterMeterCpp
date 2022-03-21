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
    Topic topic;
    int32_t payload;
};

QueueHandle_t QueueClient::createQueue(const uint16_t length) {
    if (length == 0) return nullptr;
    return xQueueCreate(length, sizeof(ShortMessage));
}

QueueClient::QueueClient(EventServer* eventServer, Log* logger, const uint16_t size, const int8_t index) :
    EventClient(eventServer),
    _logger(logger),
    // being careful with reporting on spaces as it may use them as well, so just every 5
    _freeSpaces(eventServer, Topic::FreeQueueSpaces, 5, 0, index, size),
    _receiveQueue(createQueue(size)) {
}

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
    _eventServer->publish(this, message.topic, message.payload);
    // conversion should not be a problem - values don't get large
    _freeSpaces = static_cast<long>(uxQueueSpacesAvailable(_receiveQueue));
    return true;
}

// Queue client can only handle longs, so convert strings to long (becomes 0 if that fails)
void QueueClient::update(const Topic topic, const char* payload) {
    update(topic, strtol(payload, nullptr, 10));
}

void QueueClient::update(const Topic topic, const long payload) {
    if (_sendQueue == nullptr) return;
    const ShortMessage message = {topic, static_cast<int32_t>(payload)};
    if (xQueueSendToBack(_sendQueue, &message, 0) == pdFALSE) {
        // Catch 22 - we may need a queue to send an error, and that fails. So we're using a direct log.
        // That uses the default format which gives more details 
        _logger->log("[E] Instance %p: error sending %d/%ld\n", this, topic, payload);
    }
}
