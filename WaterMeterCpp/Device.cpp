// Copyright 2021-2022 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

// ReSharper disable CppParameterMayBeConst -- does not work because of taskHandle_t definition

#include <ESP.h>

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
}

long Device::freeStack(TaskHandle_t taskHandle) {
      return static_cast<long>(uxTaskGetStackHighWaterMark(taskHandle));
}

long  Device::freeHeap() {
    return ESP.getFreeHeap();
}

void Device::reportHealth() {
    _freeHeap = freeHeap();
    if (_samplerHandle == nullptr) return;
    _freeStackSampler = freeStack(_samplerHandle);
    _freeStackCommunicator = freeStack(_communicatorHandle);
    _freeStackConnector = freeStack(_connectorHandle);
}
