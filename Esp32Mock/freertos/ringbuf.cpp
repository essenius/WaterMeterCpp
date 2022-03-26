// Copyright 2022 Rik Essenius
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

// ReSharper disable CppParameterNeverUsed
// ReSharper disable CppParameterMayBeConst
// ReSharper disable CppClangTidyClangDiagnosticVoidPointerToIntCast
// ReSharper disable CppClangTidyPerformanceNoIntToPtr

#pragma warning (disable:4302 4311 4312 26812)

#include <ESP.h>
#include <freertos/ringbuf.h>


constexpr int16_t MAX_ITEMS = 100;
constexpr int16_t MAX_ITEM_SIZE = 64;

constexpr int MAX_RINGBUFFERS = 10;


struct RingBufferContainer {
    RingbufHandle_t ringbufHandle = nullptr;
    char ringBuffer[MAX_ITEMS][2][MAX_ITEM_SIZE]{};
    size_t itemSize[MAX_ITEMS]{};
    short nextBufferItem = 0;
    short nextReadBufferItem = 0;
    bool bufferIsFull = false;
};

RingBufferContainer container[MAX_RINGBUFFERS];

int getIndex(RingbufHandle_t bufferHandle) {
    // Hack, but good enough for a mock
    return reinterpret_cast<int>(bufferHandle) - 1;
}

// testing only

void setRingBufferBufferFull(RingbufHandle_t bufferHandle, bool isFull) {
    const int i = getIndex(bufferHandle);
    container[i].bufferIsFull = isFull;
}

void uxRingbufReset() {
    for (auto& i : container) {
        i.ringbufHandle = nullptr;
        i.nextBufferItem = 0;
        i.nextReadBufferItem = 0;
        i.bufferIsFull = false;
        for (unsigned long long& j : i.itemSize) {
            j = 0;
        }
    }
}

RingbufHandle_t xRingbufferCreate(size_t xBufferSize, RingbufferType_t xBufferType) {
    int i = 0;
    while (container[i].ringbufHandle != nullptr && i <= MAX_RINGBUFFERS) {
        i++;
    }
    if (i > MAX_RINGBUFFERS) return nullptr;
    container[i].ringbufHandle = reinterpret_cast<RingbufHandle_t>(i + 1);
    return container[i].ringbufHandle;
}

size_t xRingbufferGetCurFreeSize(RingbufHandle_t bufferHandle) {
    if (bufferHandle == nullptr) return 0;
    const int i = getIndex(bufferHandle);
    if (container[i].bufferIsFull) return 7;
    return static_cast<size_t>(100 - container[i].nextBufferItem) * MAX_ITEM_SIZE * 2;
}

BaseType_t xRingbufferReceiveSplit(RingbufHandle_t bufferHandle, void** item1, void** item2, size_t* item1Size,
    size_t* item2Size, uint32_t ticksToWait) {
    if (bufferHandle == nullptr) return pdFALSE;
    const int i = getIndex(bufferHandle);
    if (container[i].nextReadBufferItem >= container[i].nextBufferItem) return pdFALSE;
    *item1 = &container[i].ringBuffer[container[i].nextReadBufferItem][0];
    if (container[i].itemSize[container[i].nextReadBufferItem] > MAX_ITEM_SIZE) {
        *item2 = &container[i].ringBuffer[container[i].nextReadBufferItem][1];
        *item1Size = MAX_ITEM_SIZE;
        *item2Size = container[i].itemSize[container[i].nextReadBufferItem] - MAX_ITEM_SIZE;
    }
    else {
        *item2 = nullptr;
        *item1Size = container[i].itemSize[container[i].nextReadBufferItem];
        *item2Size = 0;
    }
    container[i].nextReadBufferItem++;
    return pdTRUE;
}

UBaseType_t xRingbufferSend(RingbufHandle_t bufferHandle, const void* payload, size_t size, TickType_t ticksToWait) {
    if (bufferHandle == nullptr) return pdFALSE;
    const int i = getIndex(bufferHandle);
    if (container[i].nextBufferItem >= MAX_ITEMS) return pdFALSE;
    if (size > 2LL * MAX_ITEM_SIZE) return pdFALSE;
    if (size > MAX_ITEM_SIZE) {
        const auto startItem2Pointer = static_cast<const char*>(payload) + MAX_ITEM_SIZE;
        memcpy(&container[i].ringBuffer[container[i].nextBufferItem][0], payload, MAX_ITEM_SIZE);
        memcpy(&container[i].ringBuffer[container[i].nextBufferItem][1], startItem2Pointer, size - MAX_ITEM_SIZE);
    }
    else {
        memcpy(&container[i].ringBuffer[container[i].nextBufferItem][0], payload, size);
    }
    container[i].itemSize[container[i].nextBufferItem] = size;
    container[i].nextBufferItem++;
    return pdTRUE;
}
