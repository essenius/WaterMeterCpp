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

#ifndef HEADER_EVENTCLIENT
#define HEADER_EVENTCLIENT

#include <cstdarg>
#include <map>

#include "IntCoordinate.h"

constexpr long LONG_TRUE = 1L;
constexpr long LONG_FALSE = 0L;

enum class Topic: int16_t {
    None = 0,
    BatchSize,
    BatchSizeDesired,
    Rate,
    IdleRate,
    NonIdleRate,
    Sample,
    Samples,
    SkipSamples,
    SamplesFormatted,
    Result,
    ResultFormatted,
    SensorData,
    FreeHeap,
    FreeStack,
    FreeQueueSize,
    FreeQueueSpaces,
    Connection,
    WifiSummaryReady,
    ResultWritten,
    ConnectionError,
    Info,
    MessageFormatted,
    ErrorFormatted,
    Blocked,
    Alert,
    ProcessTime,
    TimeOverrun,
    Exclude,
    Pulse,
    Time,
    IpAddress,
    MacRaw,
    MacFormatted,
    ResetSensor,
    SensorWasReset,
    NoSensorFound,
    SetVolume,
    AddVolume,
    Volume,
    Pulses,
    NoDisplayFound,
    UpdateProgress,
    ButtonPushed,
    Begin,
    NoFit
};

union EventPayload {
    int32_t n;
    IntCoordinate coordinate;
    bool b;
};

class EventServer;

class EventClient {
public:
    explicit EventClient(EventServer* eventServer);
    virtual ~EventClient();
    EventClient(const EventClient&) = default;
    EventClient(EventClient&&) = default;
    EventClient& operator=(const EventClient&) = default;
    EventClient& operator=(EventClient&&) = default;

    // can't use generics on virtual functions, which we would need here.
    // Since we use only a limited set of types, type erasure seems overkill 

    virtual const char* get(Topic topic, const char* defaultValue) { return defaultValue; }
    virtual long get(Topic topic, const long defaultValue) { return defaultValue; }
    virtual void update(Topic topic, const char* payload) {}
    virtual void update(Topic topic, long payload);
    virtual void update(Topic topic, IntCoordinate payload);

protected:
    EventServer* _eventServer;
};

#endif
