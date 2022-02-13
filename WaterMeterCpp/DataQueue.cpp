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
#include "SafeCString.h"
#include "SensorDataQueuePayload.h"

#ifdef ESP32
#include <ESP.h>
#else
#include "ArduinoMock.h"
#endif

DataQueue::DataQueue(EventServer* eventServer, SensorDataQueuePayload* payload, const int8_t index, const long queueSize, const long epsilon, const long lowThreshold) :
    EventClient(eventServer),
    _bufferHandle(xRingbufferCreate(queueSize, RINGBUF_TYPE_ALLOWSPLIT)),
    _freeSpace(eventServer, Topic::FreeQueueSize, epsilon, lowThreshold, index),
    _payload(payload){}

bool DataQueue::canSend(const SensorDataQueuePayload* payload) {
    return freeSpace() >= requiredSize(payload->size());
}
 
size_t DataQueue::freeSpace() {
    const auto space = xRingbufferGetCurFreeSize(_bufferHandle);
    _freeSpace = static_cast<long>(space);
    return space;
}

RingbufHandle_t DataQueue::handle() const { return _bufferHandle; }

size_t DataQueue::requiredSize(const size_t realSize) {
    // round up to nearest 32 bit aligned size, and add an 8 byte header
    return (realSize + 3) / 4 * 4 + 8;
}

SensorDataQueuePayload* DataQueue::receive() const {
    char* item1 = nullptr;
    char* item2 = nullptr;
    size_t item1Size;
    size_t item2Size;
    const BaseType_t returnValue = xRingbufferReceiveSplit(
        _bufferHandle,
        reinterpret_cast<void**>(&item1),
        reinterpret_cast<void**>(&item2),
        &item1Size,
        &item2Size,
        0);

    if (returnValue != pdTRUE || item1 == nullptr) return nullptr;

    memcpy(_payload, item1, item1Size);
    vRingbufferReturnItem(_bufferHandle, item1);
    if (item2 != nullptr) {
        // TODO: figure out how to do this c-style cast the c++ way
        const auto targetAddress = (void*)(reinterpret_cast<const char*>(_payload) + item1Size);
        memcpy(targetAddress, item2, item2Size);
        vRingbufferReturnItem(_bufferHandle, item2);
    }
    return _payload;
}

bool DataQueue::send(const SensorDataQueuePayload* payload) {
    // optimizing the use of the buffer by not sending unused parts
    const size_t size = payload->size();
    if (requiredSize(size) > freeSpace()) return false;
    return (xRingbufferSend(_bufferHandle, payload, size, 0) == pdTRUE);
}

void DataQueue::update(Topic topic, const char* payload) {
    SensorDataQueuePayload payloadToSend{};
    payloadToSend.topic = topic;
    // timestamp is ignored for strings
    safeStrcpy(payloadToSend.buffer.message, payload);
    send(&payloadToSend);
}
