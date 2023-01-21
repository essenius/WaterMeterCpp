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

// ResultData must be smaller than Samples

struct ResultData {
    IntCoordinate lastSample;
    uint32_t sampleCount;
    uint32_t resetCount;
    uint32_t pulseCount;
    uint32_t maxStreak;
    uint32_t anomalyCount;
    uint32_t overrunCount;
    uint32_t totalDuration;
    uint32_t averageDuration;
    uint32_t maxDuration;
    Coordinate averaged;
    uint32_t searchCount;
    uint32_t skipCount;
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
    //DataQueuePayload() : topic(), buffer{} {}
};

#endif
