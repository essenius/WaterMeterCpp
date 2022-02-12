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

#include "FreeRtosMock.h"

#include <cstring>

constexpr int16_t MAX_ITEMS = 100;
constexpr int16_t MAX_ITEM_SIZE = 64;

constexpr int MAX_RINGBUFFERS = 10;

constexpr short MAX_ELEMENTS = 10;
class Queue {
public:
    short currentIndex;
    long long element[MAX_ELEMENTS];
};

constexpr short MAX_QUEUES = 5;
Queue queue[MAX_QUEUES];
QueueHandle_t queueHandle[MAX_QUEUES] = { nullptr };
short queueIndex = 0;

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
    return reinterpret_cast<int>(bufferHandle) - 1;
}

// testing only

void setRingBufferBufferFull(RingbufHandle_t bufferHandle, bool isFull) {
    const int i = getIndex(bufferHandle);
    container[i].bufferIsFull = isFull;
}

// xQueue

UBaseType_t uxQueueMessagesWaiting(QueueHandle_t xQueue) {
    return static_cast<Queue*>(xQueue)->currentIndex >= 0 ? pdTRUE : pdFALSE;
}

QueueHandle_t xQueueCreate(UBaseType_t uxQueueLength, UBaseType_t uxItemSize) {
    if (queueIndex < MAX_QUEUES) {
        queueHandle[queueIndex] = &queue[queueIndex];

        return queueHandle[queueIndex++];
    }
    return nullptr;
}

BaseType_t xQueueReceive(QueueHandle_t xQueue, void* pvBuffer, TickType_t xTicksToWait) {
    const auto queue1 = static_cast<Queue*>(xQueue);
    if (queue1->currentIndex <= 0) return pdFALSE;
    *static_cast<long long*>(pvBuffer) = queue1->element[0];
    for (short i = 0; i < queue1->currentIndex - 1; i++) {
        queue1->element[i] = queue1->element[i + 1];
    }
    queue1->currentIndex--;
    return pdTRUE;
}

BaseType_t xQueueSendToBack(QueueHandle_t xQueue, const void* pvItemToQueue, TickType_t xTicksToWait) {
    const auto queue1 = static_cast<Queue*>(xQueue);

    if (queue1->currentIndex >= MAX_ELEMENTS) return pdFALSE;
    queue1->element[queue1->currentIndex++] = *static_cast<const long long*>(pvItemToQueue);
    return pdTRUE;
}

BaseType_t xQueueSendToFront(QueueHandle_t xQueue, const void* pvItemToQueue, TickType_t xTicksToWait) { return pdFALSE; }

void uxQueueReset() {
    queueIndex = 0;
    for (short i = 0; i < MAX_QUEUES; i++) {
        queueHandle[i] = nullptr;
        queue[i].currentIndex = 0;
    }
    for (int i = 0; i < MAX_RINGBUFFERS; i++) {
        container[i].ringbufHandle = nullptr;
        container[i].nextBufferItem = 0;
        container[i].nextReadBufferItem = 0;
        container[i].bufferIsFull = false;
        for (int j = 0; j < MAX_ITEMS; j++) {
            container[i].itemSize[j] = 0;
        }
    }
}

UBaseType_t uxQueueSpacesAvailable(QueueHandle_t xQueue) {
    const auto queue1 = static_cast<Queue*>(xQueue);
    return MAX_ELEMENTS - queue1->currentIndex;
}

// Ringbuffer

RingbufHandle_t xRingbufferCreate(size_t xBufferSize, RingbufferType_t xBufferType) {
    int i = 0;
    while (container[i].ringbufHandle != nullptr && i <= MAX_RINGBUFFERS) {
        i++;
    }
    if (i > MAX_RINGBUFFERS) return nullptr;
    container[i].ringbufHandle = reinterpret_cast<RingbufHandle_t>(i+1);
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
    if (size > 2 * MAX_ITEM_SIZE) return pdFALSE;
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

// Task

unsigned long taskHandle = 100;

BaseType_t xTaskCreatePinnedToCore(TaskFunction_t pvTaskCode, const char* pcName, uint16_t usStackDepth,
                                   void* pvParameters, UBaseType_t uxPriority, TaskHandle_t* pxCreatedTask,
                                   BaseType_t xCoreID) {
    *pxCreatedTask = (TaskHandle_t)taskHandle++;
    return pdTRUE;
}

TaskHandle_t testHandle = reinterpret_cast<TaskHandle_t>(42);

TaskHandle_t xTaskGetCurrentTaskHandle() { return testHandle; }
