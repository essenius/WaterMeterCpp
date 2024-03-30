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

// We use a data queue to transport larger items between two processes - usually from Sampler to Communicator
// We do this to limit the number of times we need to send data, but also to be able to deal with incidental network glitches.

#ifndef HEADER_DATA_QUEUE
#define HEADER_DATA_QUEUE

// ReSharper disable once CppUnusedIncludeDirective -- false positive
#include <freertos/freeRTOS.h>
#include <freertos/ringbuf.h>

#include "EventClient.h"
#include "LongChangePublisher.h"
#include "DataQueuePayload.h"

namespace WaterMeter {
    class DataQueue final : public EventClient {
    public:
        DataQueue(EventServer* eventServer, DataQueuePayload* payload, int8_t index = 0, long queueSize = 35840,
            long epsilon = 1024, long lowThreshold = 2048);

        bool canSend(const DataQueuePayload* payload);
        size_t freeSpace();
        RingbufHandle_t handle() const;
        DataQueuePayload* receive() const;
        static size_t requiredSize(size_t realSize);
        bool send(const DataQueuePayload* payload);
        void update(Topic topic, const char* payload) override;

    private:
        RingbufHandle_t _bufferHandle = nullptr;
        LongChangePublisher _freeSpace;
        DataQueuePayload* _payload;
    };
}
#endif
