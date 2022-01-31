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

#include "Clock.h"
#include "EventClient.h"
#include "LongChangePublisher.h"
#include "RingbufferPayload.h"
#include "Serializer.h"


class DataQueue : public EventClient {
public:
    DataQueue(EventServer* eventServer, Clock* theClock, Serializer* serializer);

    bool canSend(const RingbufferPayload* payload);
    size_t freeSpace();
    static size_t payloadSize(const RingbufferPayload* payload);
    static size_t requiredSize(size_t realSize);

    bool send(RingbufferPayload* payload);

    bool receive() const;

private:
    LongChangePublisher _freeSpace;
    RingbufHandle_t _bufferHandle = nullptr;
    Clock* _clock;
    Serializer* _serializer;
};

#endif
