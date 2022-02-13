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
    _freeHeap(eventServer, Topic::FreeHeap, 5000L, 25000L),
    // catch all changes as this is not expected to change
    _freeStackSampler(eventServer, Topic::FreeStackSampler, 0L, 0L),
    _freeStackCommunicator(eventServer, Topic::FreeStackCommunicator, 0L, 0L),
    _freeStackConnector(eventServer, Topic::FreeStackConnector, 0L, 0L) {}

void Device::begin(
    const TaskHandle_t samplerHandle, 
    const TaskHandle_t communicatorHandle,
    const TaskHandle_t connectorHandle) {
    _samplerHandle = samplerHandle;
    _communicatorHandle = communicatorHandle;
    _connectorHandle = connectorHandle;
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
    static int count = -1;
    count++;
    if (count <= 2) return 32000L - count * 3000L;
    if (count <= 4) return 26000L;
    count = -1;
    return 23000L;
}

long Device::freeStack(TaskHandle_t taskHandle) {
    if (taskHandle == _samplerHandle) {
        static int count = -1;
        count++;
        if (count <= 2) return 1500L + count * 64L;
        if (count <= 4) return 1628L;
        count = -1;
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
