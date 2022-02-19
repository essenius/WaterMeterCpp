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

// ReSharper disable CppMemberFunctionMayBeStatic -- mimic existing interface

#ifdef ESP32
#include "ESP.h"
#else
#include "FreeRtosMock.h"
#endif

#include "Device.h"

Device::Device(EventServer* eventServer) :
    EventClient(eventServer),
    // Only catch larger variations or alarmingly low values to avoid very frequent updates
    _freeHeap(eventServer, Topic::FreeHeap, 5000L, 20000L),
    // catch all changes as this is not expected to change
    _freeStackSampler(eventServer, Topic::FreeStack, 0),
    _freeStackCommunicator(eventServer, Topic::FreeStack, 1),
    _freeStackConnector(eventServer, Topic::FreeStack, 2) {}

void Device::begin(TaskHandle_t samplerHandle, TaskHandle_t communicatorHandle, TaskHandle_t connectorHandle) {
    _samplerHandle = samplerHandle;
    _communicatorHandle = communicatorHandle;
    _connectorHandle = connectorHandle;
#ifndef ESP32
    _heapCount = -1;
    _samplerCount = -1;
#endif

}

#ifdef ESP32
// running on the device
#include <ESP.h>
//#define INCLUDE_uxTaskGetStackHighWaterMark 1

long  Device::freeHeap() {
    return ESP.getFreeHeap();
}

long Device::freeStack(TaskHandle_t taskHandle) {
      return uxTaskGetStackHighWaterMark(taskHandle);
}

#else
// mock implementation for testing. Simulate changes as well as staying the same

long Device::freeHeap() {
    _heapCount++;
    if (_heapCount <= 6) return 32000L - _heapCount * 3000L;
    if (_heapCount == 7) return 14000;
    _heapCount = -1;
    return 11000L;
}

long Device::freeStack(TaskHandle_t taskHandle) {
    if (taskHandle == _samplerHandle) {
        _samplerCount++;
        if (_samplerCount <= 2) return 1500L + _samplerCount * 64L;
        if (_samplerCount <= 4) return 1628L;
        _samplerCount = -1;
        return 1500;
    }
    if (taskHandle == _communicatorHandle) return 3750;
    return 5250;
}
#endif

void Device::reportHealth() {
    _freeHeap = freeHeap();
    if (_samplerHandle == nullptr) return;
    _freeStackSampler = freeStack(_samplerHandle);
    _freeStackCommunicator = freeStack(_communicatorHandle);
    _freeStackConnector = freeStack(_connectorHandle);
}
