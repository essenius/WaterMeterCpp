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

// Mock implementation of the used parts of the Free RTOS library for unit testing (not targeting the ESP32)

// Disabling warnings caused by mimicking existing interfaces
// ReSharper disable CppInconsistentNaming
// ReSharper disable CppParameterNeverUsed

#ifndef HEADER_FREERTOS
#define HEADER_FREERTOS

#include <climits>
#include <cstdint>

using QueueHandle_t = void*;
using TaskHandle_t = void*;

using UBaseType_t = unsigned long;
using BaseType_t = long;
using TickType_t = uint32_t;
using TaskFunction_t = void(*)(void*);

#define pdFALSE ((BaseType_t)0)
#define pdTRUE  ((BaseType_t)1)
constexpr unsigned long portMAX_DELAY = ULONG_MAX;
#define configSTACK_DEPTH_TYPE    uint16_t

#define configTICK_RATE_HZ			(1000)
#define portTICK_PERIOD_MS			((TickType_t)1000 / configTICK_RATE_HZ)
#define pdMS_TO_TICKS(xTimeInMs)    ((TickType_t)(((TickType_t)(xTimeInMs)*(TickType_t)configTICK_RATE_HZ)/(TickType_t)1000U))

QueueHandle_t xQueueCreate(UBaseType_t uxQueueLength, UBaseType_t uxItemSize);

BaseType_t xQueueReceive(QueueHandle_t xQueue, void* pvBuffer, TickType_t xTicksToWait);

BaseType_t xQueueSendToBack(QueueHandle_t xQueue, const void* pvItemToQueue, TickType_t xTicksToWait);

BaseType_t xQueueSendToFront(QueueHandle_t xQueue, const void* pvItemToQueue, TickType_t xTicksToWait);

UBaseType_t uxQueueSpacesAvailable(QueueHandle_t handle);

UBaseType_t uxQueueMessagesWaiting(QueueHandle_t xQueue);

UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t taskHandle);

void uxTaskGetStackHighWaterMarkReset();

BaseType_t xTaskCreatePinnedToCore(
    TaskFunction_t pvTaskCode,
    const char* pcName,
    configSTACK_DEPTH_TYPE usStackDepth,
    void* pvParameters,
    UBaseType_t uxPriority,
    TaskHandle_t* pxCreatedTask,
    BaseType_t xCoreID);

TaskHandle_t xTaskGetCurrentTaskHandle();

void vTaskNotifyGiveFromISR(TaskHandle_t xTaskToNotify, BaseType_t* pxHigherPriorityTaskWoken);
uint32_t ulTaskNotifyTake(BaseType_t xClearCountOnExit, TickType_t xTicksToWait);

// testing only, does not exist in FreeRTOS
void uxQueueReset();
constexpr short MaxQueues = 7;

#endif
