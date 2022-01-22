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
#include "RingbufferPayload.h"
#include "Serializer.h"


class DataQueue : public EventClient {
public:
    DataQueue(EventServer* eventServer, Serializer* serializer);
    //static RingbufHandle_t create();
    //static void begin(RingbufHandle_t bufferHandle);

    bool canSend(const RingbufferPayload* payload) const;
    size_t freeSpace() const;
    static size_t payloadSize(const RingbufferPayload* payload);
    static size_t requiredSize(size_t realSize);

    bool send(RingbufferPayload* payload) const;

/*    template<class PayloadType>
    static bool send(PayloadType* payload) {

        auto const size = sizeof(RingbufferPayload) - sizeof(Z) + sizeof(PayloadType)
        payload->header.timestamp = Clock::getTimestamp();
        return xRingbufferSend(_bufferHandle, payload, sizeof(*payload), pdMS_TO_TICKS(1000)) == pdTRUE;
    }*/

    //void handleMessage(RingbufferPayload* payload);
    bool receive() const;

private:
    RingbufHandle_t _bufferHandle = nullptr;
    Serializer* _serializer;
};

#endif
