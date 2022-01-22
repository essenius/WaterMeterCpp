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

#include "ConnectionState.h"
#include "EventServer.h"

struct ShortMessage {
    Topic topic;
    int16_t payload;
};

QueueHandle_t QueueClient::createQueue(const int length) {
    return xQueueCreate(length, sizeof(ShortMessage));
}

QueueClient::QueueClient(EventServer* eventServer): EventClient(eventServer), _receiveQueue(createQueue(10)) {}

void QueueClient::begin(QueueHandle_t sendQueue) {
    _sendQueue = sendQueue;
    _eventServer->subscribe(this, Topic::Connection);
}

QueueHandle_t QueueClient::getQueueHandle() const {
    return _receiveQueue;
}

void QueueClient::update(Topic topic, long payload) {
    if (_sendQueue == nullptr) return;
    if (topic == Topic::Connection) {
        _connected = static_cast<ConnectionState>(payload) == ConnectionState::MqttReady;
        return;
    }
    const ShortMessage message = { topic, static_cast<int16_t>(payload) };
    if (xQueueSendToBack(_sendQueue, &message, 0) == pdFALSE) {
        // TODO: handle error
    }
}

bool QueueClient::receive(const bool needsConnection) {
    if (_receiveQueue == nullptr || uxQueueMessagesWaiting(_receiveQueue) == 0) return false;
    if (needsConnection && !_connected) return false;
    ShortMessage message{};
    if (xQueueReceive(_receiveQueue, &message, 0) == pdFALSE) return false;

    _eventServer->publish(this, message.topic, message.payload);
    return true;
}
