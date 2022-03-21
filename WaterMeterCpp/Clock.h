// Copyright 2022 Rik Essenius
// 
//   Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//   except in compliance with the License. You may obtain a copy of the License at
// 
//       http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software distributed under the License
//    is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and limitations under the License.

#ifndef HEADER_CLOCK_H
#define HEADER_CLOCK_H

#include "EventServer.h"

// We omit ESP32 on purpose on Windows. the ESP32 mock defines INPUT which is also defined in windows.h.
#ifdef ESP32
#include <ESP.h>  
#include <sys/time.h>
#else
#include <ctime>
#include <FreeRtos.h>

// ReSharper disable CppInconsistentNaming -- redefining existing entity in ESP32

struct timeval {
    time_t tv_sec; // seconds 
    long tv_usec; // microseconds
};
#endif

using Timestamp = unsigned long long;

class Clock : public EventClient {
public:
    explicit Clock(EventServer* eventServer);
    void begin();
    const char* get(Topic topic, const char* defaultValue) override;

    Timestamp getTimestamp();
    bool formatTimestamp(Timestamp timestamp, char* destination, size_t size);

private:
    static constexpr int BUFFER_SIZE = 27;
    char _buffer[BUFFER_SIZE] = {0};
    static SemaphoreHandle_t _timeMutex;
    static SemaphoreHandle_t _formatTimeMutex;
};

#endif
