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
// ReSharper disable CppClangTidyPerformanceNoIntToPtr
// ReSharper disable CppInconsistentNaming
// ReSharper disable CppParameterMayBeConst
// ReSharper disable CppParameterNeverUsed

// deliberate conversion to handle and unscoped enum
#pragma warning (disable:4312 26812)

#include <ESP.h>
#include <freertos/freeRTOS.h>

constexpr short MaxElements = 20;

struct QueueEntry {
    uint8_t b[18];
};

class Queue {
public:
    short currentIndex;
    QueueEntry element[MaxElements];
};


Queue queue[MaxQueues];
QueueHandle_t queueHandle[MaxQueues] = {nullptr};
short queueIndex = 0;

// xQueue

UBaseType_t uxQueueMessagesWaiting(QueueHandle_t xQueue) {
    return static_cast<Queue*>(xQueue)->currentIndex;
}

TaskHandle_t firstWaterMarkHandle = nullptr;
TaskHandle_t secondWaterMarkHandle = nullptr;
int waterMarkSamplerCount = -1;

// test function
void uxTaskGetStackHighWaterMarkReset() {
    firstWaterMarkHandle = nullptr;
    secondWaterMarkHandle = nullptr;
    waterMarkSamplerCount = -1;
}

UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t taskHandle) {
    if (firstWaterMarkHandle == nullptr) {
        firstWaterMarkHandle = taskHandle;
    }
    if (taskHandle == firstWaterMarkHandle) {
        waterMarkSamplerCount++;
        if (waterMarkSamplerCount <= 2) return 1500L + waterMarkSamplerCount * 64L;
        if (waterMarkSamplerCount <= 4) return 1628L;
        waterMarkSamplerCount = -1;
        return 1500;
    }
    if (secondWaterMarkHandle == nullptr) {
        secondWaterMarkHandle = taskHandle;
    }
    if (taskHandle == secondWaterMarkHandle) return 3750;
    return 5250;
}

QueueHandle_t xQueueCreate(UBaseType_t uxQueueLength, UBaseType_t uxItemSize) {
    if (queueIndex < MaxQueues) {
        queueHandle[queueIndex] = &queue[queueIndex];

        return queueHandle[queueIndex++];
    }
    return nullptr;
}

BaseType_t xQueueReceive(QueueHandle_t xQueue, void* pvBuffer, TickType_t xTicksToWait) {
    const auto queue1 = static_cast<Queue*>(xQueue);
    if (queue1->currentIndex <= 0) return pdFALSE;
    *static_cast<QueueEntry*>(pvBuffer) = queue1->element[0];
    for (short i = 0; i < queue1->currentIndex - 1; i++) {
        queue1->element[i] = queue1->element[i + 1];
    }
    queue1->currentIndex--;
    return pdTRUE;
}

BaseType_t xQueueSendToBack(QueueHandle_t xQueue, const void* pvItemToQueue, TickType_t xTicksToWait) {
    const auto queue1 = static_cast<Queue*>(xQueue);

    if (queue1->currentIndex >= MaxElements) return pdFALSE;
    queue1->element[queue1->currentIndex++] = *static_cast<const QueueEntry*>(pvItemToQueue);
    return pdTRUE;
}

BaseType_t xQueueSendToFront(QueueHandle_t xQueue, const void* pvItemToQueue, TickType_t xTicksToWait) { return pdFALSE; }

void uxQueueReset() {
    queueIndex = 0;
    for (short i = 0; i < MaxQueues; i++) {
        queueHandle[i] = nullptr;
        queue[i].currentIndex = 0;
    }
}

UBaseType_t uxQueueSpacesAvailable(QueueHandle_t handle) {
    const auto queue1 = static_cast<Queue*>(handle);
    return MaxElements - queue1->currentIndex;
}

// Task

unsigned long taskHandle = 100;

BaseType_t xTaskCreatePinnedToCore(
    TaskFunction_t pvTaskCode,
    const char* pcName,
    uint16_t usStackDepth,
    void* pvParameters,
    UBaseType_t uxPriority,
    TaskHandle_t* pxCreatedTask,
    BaseType_t xCoreID) {
    *pxCreatedTask = reinterpret_cast<TaskHandle_t>(taskHandle++);
    return pdTRUE;
}

TaskHandle_t testHandle = reinterpret_cast<TaskHandle_t>(42);

TaskHandle_t xTaskGetCurrentTaskHandle() { return testHandle; }

bool taskNotifyLocked = true;

void vTaskNotifyGiveFromISR(TaskHandle_t xTaskToNotify, BaseType_t* pxHigherPriorityTaskWoken) {
    taskNotifyLocked = false;
}

uint32_t ulTaskNotifyTake(BaseType_t xClearCountOnExit, TickType_t xTicksToWait) {
    if (taskNotifyLocked) return 0;
    taskNotifyLocked = true;
    return 1;
}
