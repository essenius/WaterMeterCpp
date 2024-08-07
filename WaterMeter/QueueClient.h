// Copyright 2022-2024 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

// we use queues to send events between the different processes. Queues handle inter-process communication effectively
// Every queue client has its own queue that it receives from, and it can also send to another queue (which it doesn't own).

#ifndef HEADER_QUEUE_CLIENT
#define HEADER_QUEUE_CLIENT

#include "EventClient.h"
#include "Log.h" // exception: log from here only if buffer is full
#include "LongChangePublisher.h"

namespace WaterMeter {
    class QueueClient final : public EventClient {
    public:
        QueueClient(EventServer* eventServer, Log* logger, uint16_t size, int8_t index = 0);
        void begin(QueueHandle_t sendQueue = nullptr);
        QueueHandle_t getQueueHandle() const;
        bool receive();
        void update(Topic topic, const char* payload) override;
        void update(Topic topic, long payload) override;
    private:
        void send(Topic topic, intptr_t payload, bool isString = false);
        static QueueHandle_t createQueue(uint16_t length);
        Log* _logger;
        LongChangePublisher _freeSpaces;
        QueueHandle_t _receiveQueue;
        QueueHandle_t _sendQueue = nullptr;
        int8_t _index;
    };
}
#endif
