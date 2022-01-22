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

char ringBuffer[MAX_ITEMS][2][MAX_ITEM_SIZE];
size_t itemSize[MAX_ITEMS];

short nextBufferItem = 0;
bool bufferIsFull = false;

RingbufHandle_t ringbufHandle = nullptr;

void setRingBufferBufferFull(bool isFull) { bufferIsFull = isFull; }

RingbufHandle_t xRingbufferCreate(size_t xBufferSize, RingbufferType_t xBufferType) {
    if (ringbufHandle == nullptr) {
        ringbufHandle = &ringBuffer;
        return ringbufHandle;
    }
    return nullptr;
}

size_t xRingbufferGetCurFreeSize(RingbufHandle_t bufferHandle) {
    if (bufferIsFull) return 7;
    return static_cast<size_t>(100 - nextBufferItem) * MAX_ITEM_SIZE * 2;
}

UBaseType_t xRingbufferSend(RingbufHandle_t bufferHandle, const void* payload, size_t size, TickType_t  ticksToWait) {
    if (nextBufferItem >= MAX_ITEMS) return pdFALSE;
    if (size > 2 * MAX_ITEM_SIZE) return pdFALSE;
    if (size > MAX_ITEM_SIZE) {
        const auto startItem2Pointer = static_cast<const char*>(payload) + MAX_ITEM_SIZE;
        memcpy(&ringBuffer[nextBufferItem][0], payload, MAX_ITEM_SIZE);
        memcpy(&ringBuffer[nextBufferItem][1] , startItem2Pointer, size - MAX_ITEM_SIZE);
    } else {
        memcpy(&ringBuffer[nextBufferItem][0], payload, size);
    }
    itemSize[nextBufferItem] = size;
    nextBufferItem++;
    return pdTRUE;
}

short nextReadBufferItem = 0;

BaseType_t xRingbufferReceiveSplit(RingbufHandle_t bufferHandle, void** item1, void** item2, size_t* item1Size, size_t* item2Size, uint32_t ticksToWait) {
    if (nextReadBufferItem >= nextBufferItem) return pdFALSE;
    *item1 = &ringBuffer[nextReadBufferItem][0];
    if (itemSize[nextReadBufferItem] > MAX_ITEM_SIZE) {
        *item2 = &ringBuffer[nextReadBufferItem][1];
        *item1Size = MAX_ITEM_SIZE;
        *item2Size = itemSize[nextReadBufferItem] - MAX_ITEM_SIZE;
    }
    else {
        *item2 = nullptr;
        *item1Size = itemSize[nextReadBufferItem];
        *item2Size = 0;
    }
    nextReadBufferItem++;
    return pdTRUE;
}

class Queue {
public:
    short currentIndex;
    long element[10];
};

Queue queue1, queue2;


QueueHandle_t queueHandle1 = nullptr;
QueueHandle_t queueHandle2 = nullptr;

QueueHandle_t xQueueCreate(UBaseType_t uxQueueLength, UBaseType_t uxItemSize) {
    if (queueHandle1 == nullptr) {
        queueHandle1 = &queue1;
        return queueHandle1;
    }
    if (queueHandle2 == nullptr) {
        queueHandle2 = &queue2;
        return queueHandle2;
    }
    return nullptr;
}

UBaseType_t uxQueueMessagesWaiting(QueueHandle_t xQueue) { return ((Queue*)xQueue)->currentIndex>=0 ? pdTRUE : pdFALSE; }
BaseType_t xQueueReceive(QueueHandle_t xQueue, void* pvBuffer, TickType_t xTicksToWait) {
    const auto queue = static_cast<Queue*>(xQueue);
    if (queue->currentIndex <= 0) return pdFALSE;
    *static_cast<long*>(pvBuffer) = queue->element[0];
    for (short i=0; i< queue->currentIndex - 1; i++ ) {
        queue->element[i] = queue->element[i + 1];
    }
    queue->currentIndex--;
    return pdTRUE;
}

BaseType_t xQueueSendToToFront(QueueHandle_t xQueue, const void* pvItemToQueue, TickType_t xTicksToWait) { return pdFALSE; }
BaseType_t xQueueSendToBack(QueueHandle_t xQueue, const void* pvItemToQueue, TickType_t xTicksToWait) {
    const auto queue = static_cast<Queue*>(xQueue);

    if (queue->currentIndex > 9) return pdFALSE;
    queue->element[queue->currentIndex++] = *static_cast<const long*>(pvItemToQueue);
    return pdTRUE;
}

BaseType_t xTaskCreatePinnedToCore(TaskFunction_t pvTaskCode, const char* pcName, uint16_t usStackDepth,
    void* pvParameters, UBaseType_t uxPriority, TaskHandle_t* pxCreatedTask, BaseType_t xCoreID) {
    return pdTRUE;
}


void uxQueueReset() {
    queueHandle1 = nullptr;
    queueHandle2 = nullptr;
}
