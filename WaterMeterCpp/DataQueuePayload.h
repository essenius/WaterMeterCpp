// Copyright 2021-2022 Rik Essenius
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

constexpr uint16_t MAX_SAMPLES = 50;

struct Samples {
    uint16_t count;
    int16_t value[MAX_SAMPLES];
};

// ResultData must be smaller than Samples

struct ResultData {
    int16_t lastSample;
    uint32_t sampleCount;
    uint32_t resetCount;
    uint32_t peakCount;
    uint32_t flowCount;
    uint32_t maxStreak;
    uint32_t outlierCount;
    uint32_t excludeCount;
    uint32_t overrunCount;
    uint32_t totalDuration;
    uint32_t averageDuration;
    uint32_t maxDuration;
    float fastSmooth;
    float fastDerivative;
    float smoothFastDerivative;
    float smoothAbsFastDerivative;
    float slowSmooth;
    float combinedDerivative;
    float smoothAbsCombinedDerivative;
};

typedef union {
    Samples samples;
    ResultData result;
    char message[sizeof(Samples)];
    uint32_t value;
} Content; 

struct DataQueuePayload  {
    Topic topic;
    Timestamp timestamp{};
    Content buffer;
    size_t size() const ;
    DataQueuePayload() : topic(), buffer() {}
};

#endif
