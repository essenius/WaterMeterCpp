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

#ifdef ESP32
#include <ESP.h>  
#include <sys/time.h>
#else
#include <ctime>
#include <cstdio>
#endif

#include "TimeServer.h"
#include <cstring>

char TimeServer::_buffer[BUFFER_SIZE] = {'0'};

TimeServer::TimeServer(EventServer* eventServer) : EventClient("TimeServer", eventServer) {}

void TimeServer::begin() {
    if (setTime()) {
        _eventServer->provides(this, Topic::Time);
    }
    else {
        _eventServer->publish(Topic::Error, "Could not set time");
    }
}

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
//#include <cstdint>

struct timeval {
    time_t tv_sec; // seconds 
    long tv_usec; // microseconds
};

bool TimeServer::setTime() {
    _wasSet = true;
    return _wasSet;
}

int gettimeofday(struct timeval* timeVal, void* ignore) {
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

#ifdef ESP32
const time_t ONE_YEAR_IN_SECONDS = 31536000;

bool TimeServer::setTime() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    time_t currentTime = time(nullptr);
    Serial.print("Waiting for NTP time sync: ");

    // If the time didn't get set, the system thinks we're in 1970.
    // Then currentTime will be less than a year in seconds.
    int sampleCount = 0;
    while (currentTime < ONE_YEAR_IN_SECONDS && sampleCount <= 100) {
        Serial.print(++sampleCount % 10 == 0 ? "|" : ".");
        currentTime = time(nullptr);
        delay(100);
    }
    tm* timeGmt = gmtime(&currentTime);
    Serial.printf("Time: %s\n", asctime(timeGmt));
    _wasSet = sampleCount <= 100;
    return _wasSet;
}
#endif

const char* TimeServer::getTime() {
    struct timeval currentTime{};
    gettimeofday(&currentTime, nullptr);
    const long micros = currentTime.tv_usec;
    const time_t seconds = currentTime.tv_sec;
    strftime(_buffer, BUFFER_SIZE, "%Y-%m-%dT%H:%M:%S.", gmtime(&seconds));
    char* currentPosition = _buffer + strlen(_buffer);
    sprintf(currentPosition, "%06ld", micros);
    return _buffer;
}

const char* TimeServer::get(Topic topic, const char* defaultValue) {
    if (topic == Topic::Time) {
        return getTime();
    }
    return defaultValue;
}

bool TimeServer::timeWasSet() const {
    return _wasSet;
}
