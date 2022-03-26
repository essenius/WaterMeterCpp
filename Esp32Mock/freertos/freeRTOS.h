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

// Disabling warnings caused by mimicking existing interfaces
// ReSharper disable CppInconsistentNaming
// ReSharper disable CppParameterNeverUsed

#ifndef HEADER_FREERTOS
#define HEADER_FREERTOS

#include <climits>
#include <cstdint>

using QueueHandle_t = void*;
using TaskHandle_t = void*;
//using RingbufHandle_t = void*;
using SemaphoreHandle_t = void*;

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

/*
enum RingbufferType_t { RINGBUF_TYPE_NOSPLIT = 0, RINGBUF_TYPE_ALLOWSPLIT, RINGBUF_TYPE_BYTEBUF, RINGBUF_TYPE_MAX };

 test function
void setRingBufferBufferFull(RingbufHandle_t bufferHandle, bool isFull);
*/

QueueHandle_t xQueueCreate(UBaseType_t uxQueueLength, UBaseType_t uxItemSize);

BaseType_t xQueueReceive(QueueHandle_t xQueue, void* pvBuffer, TickType_t xTicksToWait);

BaseType_t xQueueSendToBack(QueueHandle_t xQueue, const void* pvItemToQueue, TickType_t xTicksToWait);

BaseType_t xQueueSendToFront(QueueHandle_t xQueue, const void* pvItemToQueue, TickType_t xTicksToWait);

UBaseType_t uxQueueSpacesAvailable(QueueHandle_t handle);

UBaseType_t uxQueueMessagesWaiting(QueueHandle_t xQueue);

UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t taskHandle);

void uxTaskGetStackHighWaterMarkReset();

/*
inline void vRingbufferReturnItem(RingbufHandle_t bufferHandle, void* item1) {}

RingbufHandle_t xRingbufferCreate(size_t xBufferSize, RingbufferType_t xBufferType);

size_t xRingbufferGetCurFreeSize(RingbufHandle_t bufferHandle);

BaseType_t xRingbufferReceiveSplit(RingbufHandle_t bufferHandle, void** item1, void** item2, size_t* item1Size,
                                   size_t* item2Size, uint32_t ticksToWait);

UBaseType_t xRingbufferSend(RingbufHandle_t bufferHandle, const void* payload, size_t size, TickType_t ticksToWait);
*/

inline SemaphoreHandle_t xSemaphoreCreateMutex() { return nullptr; }
inline void xSemaphoreTake(SemaphoreHandle_t handle, unsigned long delay) {}
inline void xSemaphoreGive(SemaphoreHandle_t handle) {}

BaseType_t xTaskCreatePinnedToCore(TaskFunction_t pvTaskCode, const char* pcName, configSTACK_DEPTH_TYPE usStackDepth,
                                   void* pvParameters, UBaseType_t uxPriority, TaskHandle_t* pxCreatedTask,
                                   BaseType_t xCoreID);

TaskHandle_t xTaskGetCurrentTaskHandle();

// testing only, does not exist in FreeRTOS
void uxQueueReset();

#endif
