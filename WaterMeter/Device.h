// Copyright 2021-2023 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

// Provides several metrics from the device itself, to monitor health.

#ifndef HEADER_DEVICE
#define HEADER_DEVICE

#include <ESP.h>
#include "LongChangePublisher.h"

namespace WaterMeter {
    class Device final : public EventClient {
    public:
        explicit Device(EventServer* eventServer);
        void begin(TaskHandle_t samplerHandle, TaskHandle_t communicatorHandle, TaskHandle_t connectorHandle);
        void reportHealth();
    private:
        TaskHandle_t _samplerHandle{};
        TaskHandle_t _communicatorHandle{};
        TaskHandle_t _connectorHandle{};
        LongChangePublisher _freeHeap;
        ChangePublisher<long> _freeStackSampler;
        ChangePublisher<long> _freeStackCommunicator;
        ChangePublisher<long> _freeStackConnector;

        static long freeHeap();
        static long freeStack(TaskHandle_t taskHandle);
    };
}
#endif
