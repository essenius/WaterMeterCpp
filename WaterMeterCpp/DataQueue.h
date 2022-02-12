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

#ifndef HEADER_DATAQUEUE_H
#define HEADER_DATAQUEUE_H

#ifdef ESP32
#include "freertos/freeRTOS.h"
#include "freertos/ringbuf.h"
#else
#include "FreeRtosMock.h"
#endif

#include "EventClient.h"
#include "SensorDataQueuePayload.h"

class DataQueue : public EventClient {
public:
    DataQueue(EventServer* eventServer, SensorDataQueuePayload* payload, size_t queueSize);

    bool canSend(const SensorDataQueuePayload* payload);

    virtual size_t freeSpace();
    static size_t requiredSize(size_t realSize);

    bool send(const SensorDataQueuePayload* payload); 
    void update(Topic topic, const char* payload) override;

    SensorDataQueuePayload* receive() const;
    RingbufHandle_t handle() const;

private:
    RingbufHandle_t _bufferHandle = nullptr;
    SensorDataQueuePayload* _payload;
};

#endif
