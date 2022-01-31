// Copyright 2022 Rik Essenius
// 
//   Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//   except in compliance with the License. You may obtain a copy of the License at
// 
//       http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software distributed under the License
//    is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and limitations under the License.

#include "QueueClient.h"
#include "EventServer.h"

struct ShortMessage {
    Topic topic;
    int32_t payload;
};

QueueHandle_t QueueClient::createQueue(const uint16_t length) {
    return xQueueCreate(length, sizeof(ShortMessage));
}

QueueClient::QueueClient(EventServer* eventServer, uint16_t size): EventClient(eventServer), _receiveQueue(createQueue(size)) {}

void QueueClient::begin(QueueHandle_t sendQueue) {
    _sendQueue = sendQueue;
}

QueueHandle_t QueueClient::getQueueHandle() const {
    return _receiveQueue;
}

void QueueClient::update(Topic topic, long payload) {
    if (_sendQueue == nullptr) return;
    const ShortMessage message = {topic, static_cast<int32_t>(payload)};
    if (xQueueSendToBack(_sendQueue, &message, 0) == pdFALSE) {
        // TODO: handle error
    }
}

bool QueueClient::receive() {
    if (_receiveQueue == nullptr || uxQueueMessagesWaiting(_receiveQueue) == 0) return false;
    ShortMessage message{};
    if (xQueueReceive(_receiveQueue, &message, 0) == pdFALSE) return false;
    _eventServer->publish(this, message.topic, message.payload);
    return true;
}
