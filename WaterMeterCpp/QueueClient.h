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

#ifndef HEADER_QUEUECLIENT
#define HEADER_QUEUECLIENT

#ifdef ESP32
#include <ESP.h>
#else
#include "FreeRtosMock.h"
#include "ArduinoMock.h"
#endif

#include "EventClient.h"
#include "LongChangePublisher.h"

class QueueClient : public EventClient {
public:
    QueueClient(EventServer* eventServer, uint16_t size, int8_t index = 0);
    void begin(QueueHandle_t sendQueue = nullptr);
    QueueHandle_t getQueueHandle() const;
    bool receive();
    void update(Topic topic, const char* payload) override;
    void update(Topic topic, long payload) override;
private:
    static QueueHandle_t createQueue(uint16_t length);
    LongChangePublisher _freeSpaces;
    QueueHandle_t _receiveQueue;
    QueueHandle_t _sendQueue = nullptr;
};

#endif
