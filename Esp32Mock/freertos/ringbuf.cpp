// Copyright 2022-2023 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

// Mock implementation for unit testing (not targeting the ESP32)

// Disabling warnings caused by mimicking existing interfaces or mocking hacks

// ReSharper disable CppParameterMayBeConst

#include <ESP.h>
#include <freertos/ringbuf.h>
#include <limits>

constexpr int16_t MaxItems = 100;
constexpr int16_t MaxItemSize = 64;
constexpr int MaxRingbuffers = 10;

struct RingBufferContainer {
    RingbufHandle_t ringbufHandle = nullptr;
    char ringBuffer[MaxItems][2][MaxItemSize]{};
    size_t itemSize[MaxItems]{};
    short nextBufferItem = 0;
    short nextReadBufferItem = 0;
    bool bufferIsFull = false;
};

RingBufferContainer container[MaxRingbuffers];

int getIndex(RingbufHandle_t bufferHandle) {
    // Match the handle size to an int, and subtract 1 to get the index
    const uintptr_t handle = reinterpret_cast<uintptr_t>(bufferHandle); 
    return static_cast<int>(handle & std::numeric_limits<int>::max()) - 1;
}

// testing only

void setRingBufferBufferFull(RingbufHandle_t bufferHandle, bool isFull) {
    const int i = getIndex(bufferHandle);
    container[i].bufferIsFull = isFull;
}

void setRingBufferNoMoreEntries(RingbufHandle_t bufferHandle) {
    const int i = getIndex(bufferHandle);
    container[i].nextBufferItem = MaxItems + 1;
}

void uxRingbufReset() {
    for (auto& i : container) {
        i.ringbufHandle = nullptr;
        i.nextBufferItem = 0;
        i.nextReadBufferItem = 0;
        i.bufferIsFull = false;
        for (size_t& j : i.itemSize) {
            j = 0;
        }
    }
}

RingbufHandle_t xRingbufferCreate(size_t /*xBufferSize*/, RingbufferType_t /*xBufferType*/) {
    int i = 0;
    while (container[i].ringbufHandle != nullptr && i <= MaxRingbuffers) {
        i++;
    }
    if (i > MaxRingbuffers) return nullptr;
    const auto pointer = static_cast<uintptr_t>(i + 1);
    container[i].ringbufHandle = reinterpret_cast<RingbufHandle_t>(pointer);
    return container[i].ringbufHandle;
}

size_t xRingbufferGetCurFreeSize(RingbufHandle_t bufferHandle) {
    if (bufferHandle == nullptr) return 0;
    const int i = getIndex(bufferHandle);
    if (container[i].bufferIsFull) return 7;
    return static_cast<size_t>(100 - container[i].nextBufferItem) * MaxItemSize * 2;
}

BaseType_t xRingbufferReceiveSplit(RingbufHandle_t bufferHandle, void** item1, void** item2, size_t* item1Size,
                                   size_t* item2Size, uint32_t /*ticksToWait*/) {
    if (bufferHandle == nullptr) return pdFALSE;
    const int i = getIndex(bufferHandle);
    if (container[i].nextReadBufferItem >= container[i].nextBufferItem) return pdFALSE;
    *item1 = &container[i].ringBuffer[container[i].nextReadBufferItem][0];
    if (container[i].itemSize[container[i].nextReadBufferItem] > MaxItemSize) {
        *item2 = &container[i].ringBuffer[container[i].nextReadBufferItem][1];
        *item1Size = MaxItemSize;
        *item2Size = container[i].itemSize[container[i].nextReadBufferItem] - MaxItemSize;
    }
    else {
        *item2 = nullptr;
        *item1Size = container[i].itemSize[container[i].nextReadBufferItem];
        *item2Size = 0;
    }
    container[i].nextReadBufferItem++;
    return pdTRUE;
}

UBaseType_t xRingbufferSend(RingbufHandle_t bufferHandle, const void* payload, size_t size, TickType_t /*ticksToWait*/) {
    if (bufferHandle == nullptr) return pdFALSE;
    const int i = getIndex(bufferHandle);
    if (container[i].nextBufferItem >= MaxItems) return pdFALSE;
    if (size > 2LL * MaxItemSize) return pdFALSE;
    if (size > MaxItemSize) {
        const auto startItem2Pointer = static_cast<const char*>(payload) + MaxItemSize;
        memcpy(&container[i].ringBuffer[container[i].nextBufferItem][0], payload, MaxItemSize);
        memcpy(&container[i].ringBuffer[container[i].nextBufferItem][1], startItem2Pointer, size - MaxItemSize);
    }
    else {
        memcpy(&container[i].ringBuffer[container[i].nextBufferItem][0], payload, size);
    }
    container[i].itemSize[container[i].nextBufferItem] = size;
    container[i].nextBufferItem++;
    return pdTRUE;
}
