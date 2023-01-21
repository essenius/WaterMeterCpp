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

#include "Clock.h"
#include <sys/time.h>
#include <cstring>

constexpr unsigned long long MICROSECONDS_PER_SECOND = 1000000ULL;

SemaphoreHandle_t Clock::_timeMutex = xSemaphoreCreateMutex();
SemaphoreHandle_t Clock::_formatTimeMutex = xSemaphoreCreateMutex();

Clock::Clock(EventServer* eventServer) : EventClient(eventServer) {}

void Clock::begin() {
    _eventServer->provides(this, Topic::Time);
}

// return the number of microseconds since epoch. Can be called from multipe tasks, so using a semaphore
Timestamp Clock::getTimestamp() {
    timeval currentTime{};
    xSemaphoreTake(_timeMutex, portMAX_DELAY);
    gettimeofday(&currentTime, nullptr);
    xSemaphoreGive(_timeMutex);
    return static_cast<Timestamp>(currentTime.tv_sec) * 1000000ULL + static_cast<Timestamp>(currentTime.tv_usec);
}

bool Clock::formatTimestamp(const Timestamp timestamp, char* destination, const size_t size) {
    if (size < 27) return false;
    const auto microseconds = static_cast<long>(timestamp % MICROSECONDS_PER_SECOND);
    const auto seconds = static_cast<time_t>(timestamp / MICROSECONDS_PER_SECOND);
    xSemaphoreTake(_formatTimeMutex, portMAX_DELAY);
    strftime(destination, size, "%Y-%m-%dT%H:%M:%S.", gmtime(&seconds)); // NOLINT(concurrency-mt-unsafe)
    xSemaphoreGive(_formatTimeMutex);
    char* currentPosition = destination + strlen(destination);
    snprintf(currentPosition, size - strlen(destination), "%06ld", microseconds);
    return true;
}

const char* Clock::get(const Topic topic, const char* defaultValue) {
    if (topic == Topic::Time) {
        const auto currentTime = getTimestamp();
        formatTimestamp(currentTime, _buffer, BUFFER_SIZE);
        return _buffer;
    }
    return defaultValue;
}
