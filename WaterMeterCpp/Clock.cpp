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


#include "Clock.h"

#include "FreeRtosMock.h"

constexpr unsigned long long MICROSECONDS_PER_SECOND = 1000000ULL;

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

int gettimeofday(timeval* timeVal, void* ignore) {
    if (timeVal) {
        FILETIME filetime; // 0.1 microsecond intervals since January 1, 1601 00:00 UTC 
        ULARGE_INTEGER x = {{0, 0}};
        ULONGLONG usec;
        static constexpr ULONGLONG EPOCH_OFFSET_MICROS = 11644473600000000ULL;
        // microseconds betweeen Jan 1,1601 and Jan 1,1970 

#if _WIN32_WINNT >= _WIN32_WINNT_WIN8
        GetSystemTimePreciseAsFileTime(&filetime);
#else
        GetSystemTimeAsFileTime(&filetime);
#endif
        x.LowPart = filetime.dwLowDateTime;
        x.HighPart = filetime.dwHighDateTime;
        usec = x.QuadPart / 10 - EPOCH_OFFSET_MICROS;
        timeVal->tv_sec = static_cast<time_t>(usec / 1000000ULL);
        timeVal->tv_usec = static_cast<long>(usec % 1000000ULL);
    }
    return 0;
}
#endif

SemaphoreHandle_t Clock::_timeMutex = xSemaphoreCreateMutex();
SemaphoreHandle_t Clock::_formatTimeMutex = xSemaphoreCreateMutex();

Clock::Clock(EventServer* eventServer) : EventClient(eventServer) {}

void Clock::begin() {
    _eventServer->provides(this, Topic::Time);
}

// return the number of microseconds since epoch. Can be called from multipe tasks, so using a semaphore
Timestamp Clock::getTimestamp() {
    timeval currentTime{};
    xSemaphoreTake(Clock::_timeMutex, portMAX_DELAY);
    gettimeofday(&currentTime, nullptr);
    xSemaphoreGive(Clock::_timeMutex);
    return static_cast<Timestamp>(currentTime.tv_sec) * 1000000ULL + static_cast<Timestamp>(currentTime.tv_usec);
}

bool Clock::formatTimestamp(Timestamp timestamp, char* destination, size_t size) {
    if (size < 27) return false;
    const auto currentTime = getTimestamp();
    const auto microseconds = static_cast<long>(currentTime % MICROSECONDS_PER_SECOND);
    const auto seconds = static_cast<time_t>(currentTime / MICROSECONDS_PER_SECOND);
    xSemaphoreTake(Clock::_formatTimeMutex, portMAX_DELAY);    
    strftime(destination, size, "%Y-%m-%dT%H:%M:%S.", gmtime(&seconds));
    xSemaphoreGive(Clock::_formatTimeMutex);    
    char* currentPosition = destination + strlen(destination);
    snprintf(currentPosition, size - strlen(destination), "%06ld", microseconds);
    return true;
}

const char* Clock::get(Topic topic, const char* defaultValue) {
    if (topic == Topic::Time) {
        const auto currentTime = getTimestamp();
        formatTimestamp(currentTime, _buffer, BUFFER_SIZE);
        return _buffer;
    }
    return defaultValue;
}
