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

#include "EventServer.h"
#include "DataQueue.h"
#include <cstring>

#ifdef ESP32
#include <ESP.h>
#else
#include "ArduinoMock.h"
#endif

DataQueue::DataQueue(EventServer* eventServer, Clock* theClock, Serializer* serializer) :
    EventClient(eventServer),
    _bufferHandle(xRingbufferCreate(40960, RINGBUF_TYPE_ALLOWSPLIT)),
    _clock(theClock),
    _serializer(serializer) {}

bool DataQueue::canSend(const RingbufferPayload* payload) const {
    return freeSpace() >= requiredSize(payloadSize(payload));
}

size_t DataQueue::freeSpace() const {
    return xRingbufferGetCurFreeSize(_bufferHandle);
}

size_t DataQueue::payloadSize(const RingbufferPayload* payload) {
    // optimizing the use of the buffer by not sending unused parts
    size_t size = sizeof(RingbufferPayload) - sizeof(Content);
    switch (payload->topic) {
    case Topic::Result:
        size += sizeof(ResultData);
        break;
    case Topic::Samples:
        size += sizeof(Samples::count) + payload->buffer.samples.count * sizeof(Samples::value[0]);
        break;
    case Topic::SamplingError:
    case Topic::Info:
        size += strlen(payload->buffer.message);
        break;
    default:
        break;
    } 
    return size;
}

size_t DataQueue::requiredSize(size_t realSize) {
    // round up to nearest 32 bit aligned size, and add an 8 byte header
    return (realSize + 3) / 4 * 4 + 8;
}

bool DataQueue::send(RingbufferPayload* payload) const {
    // optimizing the use of the buffer by not sending unused parts
    const size_t size = payloadSize(payload);
    if (requiredSize(size) > freeSpace()) return false;
    if (payload->timestamp == 0) payload->timestamp = _clock->getTimestamp();
    return (xRingbufferSend(_bufferHandle, payload, size, 0) == pdTRUE);
}

bool DataQueue::receive() const {
    char* item1 = nullptr;
    char* item2 = nullptr;
    RingbufferPayload payload{};
    size_t item1Size;
    size_t item2Size;
    const BaseType_t returnValue = xRingbufferReceiveSplit(
        _bufferHandle,
        reinterpret_cast<void**>(&item1),
        reinterpret_cast<void**>(&item2),
        &item1Size,
        &item2Size,
        0);

    if (returnValue != pdTRUE || item1 == nullptr) return false;

    memcpy(&payload, item1, item1Size);
    vRingbufferReturnItem(_bufferHandle, item1);
    if (item2 != nullptr) {
        // TODO: figure out how to do this c-style cast the c++ way
        const auto targetAddress = (void*)(reinterpret_cast<const char*>(&payload) + item1Size);
        memcpy(targetAddress, item2, item2Size);
        vRingbufferReturnItem(_bufferHandle, item2);
    }

    const char* message = _serializer->convertPayload(&payload);
    _eventServer->publish(payload.topic, message);
    return true;
}
