// Copyright 2021 Rik Essenius
// 
//   Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//   except in compliance with the License. You may obtain a copy of the License at
// 
//       http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software distributed under the License
//    is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and limitations under the License.

#include "Device.h"
#include <cstdlib>

Device::Device(EventServer* eventServer) : EventClient("Device", eventServer) {}

void Device::begin() {
    _eventServer->subscribe(this, Topic::Error);
    _eventServer->subscribe(this, Topic::Info);
    reportHealth();
}

#ifdef ESP32
#include <ESP.h>
#define INCLUDE_uxTaskGetStackHighWaterMark 1

long  Device::freeHeap() {
    return ESP.getFreeHeap();
}

long  Device::freeStack() {
    return uxTaskGetStackHighWaterMark(nullptr);
}
#else
#include "ArduinoMock.h"

long  Device::freeHeap() {
    static int count = 0;
    if (count > 2) count = 0;
    return 25000L + (count++) * 2500L;
}

long  Device::freeStack() {
    static int count = 0;
    if (count > 2) count = 0;
    return 1500L + (count++) * 64L;
}
#endif

void Device::reportFreeHeap() {
    long newFreeHeap = freeHeap(); // it is an uint32 but we need a sign.
    // Only catch larger variations to avoid very frequent updates
    if (abs(_freeHeap - newFreeHeap) >= 5000 || newFreeHeap < 25000) {
        _eventServer->publish(Topic::FreeHeap, newFreeHeap);
        _freeHeap = newFreeHeap;
    }
}

void Device::reportFreeStack() {
    auto newFreeStack = freeStack();
    // Not expected to change, so report any variations
    if (_freeStack != newFreeStack) {
        _eventServer->publish(Topic::FreeStack, newFreeStack);
        _freeStack = newFreeStack;
    }
}

void Device::reportHealth() {
    reportFreeHeap();
    reportFreeStack();
}
