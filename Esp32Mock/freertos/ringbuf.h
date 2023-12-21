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

// Mock implementation of the esp32 ring buffer library for unit testing (not targeting the ESP32)

// Disabling warnings caused by mimicking existing interfaces
// ReSharper disable CppInconsistentNaming
// ReSharper disable CppParameterNeverUsed

#ifndef HEADER_RINGBUF
#define HEADER_RINGBUF

using RingbufHandle_t = void*;

enum RingbufferType_t { RINGBUF_TYPE_NOSPLIT = 0, RINGBUF_TYPE_ALLOWSPLIT, RINGBUF_TYPE_BYTEBUF, RINGBUF_TYPE_MAX };

// these 3 are for testing only
void setRingBufferBufferFull(RingbufHandle_t bufferHandle, bool isFull);
void setRingBufferNoMoreEntries(RingbufHandle_t bufferHandle);
void uxRingbufReset();

inline void vRingbufferReturnItem(RingbufHandle_t bufferHandle, void* item1) {}

RingbufHandle_t xRingbufferCreate(size_t xBufferSize, RingbufferType_t xBufferType);

size_t xRingbufferGetCurFreeSize(RingbufHandle_t bufferHandle);

BaseType_t xRingbufferReceiveSplit(
    RingbufHandle_t bufferHandle,
    void** item1,
    void** item2,
    size_t* item1Size,
    size_t* item2Size,
    uint32_t ticksToWait);

UBaseType_t xRingbufferSend(RingbufHandle_t bufferHandle, const void* payload, size_t size, TickType_t ticksToWait);

#endif
