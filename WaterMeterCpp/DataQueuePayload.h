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

// The payload structure for the data queues. We use a union so we can reuse the same data queue for different formats.

#ifndef HEADER_DATAQUEUEPAYLOAD
#define HEADER_DATAQUEUEPAYLOAD

#include <cstdint>

#include "Clock.h"
#include "EventClient.h"
#include "IntCoordinate.h"

constexpr uint16_t MAX_SAMPLES = 25;

struct Samples {
    uint16_t count;
    IntCoordinate value[MAX_SAMPLES];
};

// ResultData size must be <= Samples size

struct ResultData {
    IntCoordinate lastSample;
    uint32_t sampleCount;
    uint32_t skipCount;
    uint16_t resetCount;
    uint16_t maxStreak;
    IntCoordinate ellipseCenterTimes10;
    IntCoordinate ellipseRadiusTimes10;
    int16_t ellipseAngleTimes10;
    uint16_t pulseCount;
    uint32_t anomalyCount;
    uint32_t overrunCount;
    uint32_t totalDuration;
    uint32_t averageDuration;
    uint32_t maxDuration;
};

union Content {
    Samples samples{};
    ResultData result;
    char message[sizeof(Samples)];
    uint32_t value;
};

struct DataQueuePayload {
    Topic topic{};
    Timestamp timestamp{};
    Content buffer{};
    size_t size() const;
};

#endif
