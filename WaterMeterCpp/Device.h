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

#ifndef HEADER_DEVICE_H
#define HEADER_DEVICE_H

#ifdef ESP32
#include "ESP.h"
#else
#include "FreeRtosMock.h"
#endif

#include "EventServer.h"
#include "LongChangePublisher.h"

class Device : public EventClient {
public:
    explicit Device(EventServer* eventServer);
    void begin(const TaskHandle_t& samplerHandle, const TaskHandle_t& communicatorHandle, const TaskHandle_t& connectorHandle);
    void reportHealth();
private:
    TaskHandle_t _samplerHandle{};
    TaskHandle_t _communicatorHandle{};
    TaskHandle_t _connectorHandle{};
    LongChangePublisher _freeHeap;
    LongChangePublisher _freeStackSampler;
    LongChangePublisher _freeStackCommunicator;
    LongChangePublisher _freeStackConnector;

    long freeHeap();
    long freeStack(TaskHandle_t taskHandle = nullptr);
};
#endif
