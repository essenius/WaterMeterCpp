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
#include "ArduinoMock.h"
#endif

#include "Clock.h"
#include "TimeServer.h"

char TimeServer::_buffer[BUFFER_SIZE] = {'0'};

TimeServer::TimeServer(EventServer* eventServer) : EventClient(eventServer) {}

void TimeServer::begin() {
    setTime();
    _eventServer->provides(this, Topic::Time);
}

constexpr time_t ONE_YEAR_IN_SECONDS = 31536000;

void TimeServer::setTime() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    time_t currentTime = time(nullptr);
}

bool TimeServer::timeWasSet() const {
    // If the time didn't get set, the system thinks we're in 1970.
    // Then currentTime will be less than a year in seconds.

    const time_t currentTime = time(nullptr);
    return currentTime > ONE_YEAR_IN_SECONDS;
}

const char* TimeServer::get(Topic topic, const char* defaultValue) {
    if (topic == Topic::Time) {
        const auto currentTime = Clock::getTimestamp();
        Clock::formatTimestamp(currentTime, _buffer, BUFFER_SIZE);
        return _buffer;
    }
    return defaultValue;
}

