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

#include "Device.h"

Device::Device(EventServer* eventServer) :
    EventClient(eventServer),
    // Only catch larger variations or alarmingly low values to avoid very frequent updates
    _freeHeap(eventServer, this, Topic::FreeHeap, 5000L, 25000L),
    // catch all changes as this is not expected to change
    _freeStack(eventServer, this, Topic::FreeStack, 0L, 0L) {}

void Device::begin() {
    reportHealth();
}

#ifdef ESP32
// running on the device
#include <ESP.h>
#define INCLUDE_uxTaskGetStackHighWaterMark 1

long  Device::freeHeap() {
    return ESP.getFreeHeap();
}

long  Device::freeStack() {
    return uxTaskGetStackHighWaterMark(nullptr);
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

long Device::freeStack() {
    static int count = -1;
    count++;
    if (count <= 2) return 1500L + count * 64L;
    if (count <= 4) return 1628L;
    count = -1;
    return 1500;
}
#endif

void Device::reportHealth() {
    _freeHeap.set(freeHeap());
    _freeStack.set(freeStack());
}
