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

// Get the current time and format it appropriately. We use a mutex to ensure different processes do not do this simultaneously.

#ifndef HEADER_CLOCK
#define HEADER_CLOCK

#include "Clock.h"
#include "EventClient.h"
// ReSharper disable once CppUnusedIncludeDirective -- false positive
#include "EventServer.h"

// We circumvent ESP.h - it defines INPUT which is also defined in windows.h, which time.h uses there
// ReSharper disable once CppUnusedIncludeDirective -- semphr.h requires freeRTOS.h
#include <freertos/freeRTOS.h>
#include <freertos/semphr.h>

using Timestamp = unsigned long long;

class Clock final : public EventClient {
public:
    explicit Clock(EventServer* eventServer);
    void begin();
    const char* get(Topic topic, const char* defaultValue) override;

    static Timestamp getTimestamp();
    static bool formatTimestamp(Timestamp timestamp, char* destination, size_t size);

private:
    static constexpr int BufferSize = 27;
    char _buffer[BufferSize] = {0};
    static SemaphoreHandle_t _timeMutex;
    static SemaphoreHandle_t _formatTimeMutex;
};

#endif
