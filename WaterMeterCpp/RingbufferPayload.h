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

#ifndef HEADER_RINGBUFFERPAYLOAD
#define HEADER_RINGBUFFERPAYLOAD

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
    int16_t lastSample;      //2
    uint32_t sampleCount;    //4
    uint32_t peakCount;      //4
    uint32_t flowCount;      //4
    uint32_t maxStreak;      //4
    uint32_t outlierCount;   //2
    uint32_t excludeCount;   //2
    uint32_t overrunCount;   //2
    uint32_t totalDuration;  //4
    uint32_t averageDuration;//4
    uint32_t maxDuration;    //4
    float smooth;            //4
    float derivativeSmooth;  //4
    float smoothDerivativeSmooth;    //4
    float smoothAbsDerivativeSmooth; //4
};

typedef union {
    Samples samples;
    ResultData result;
    char message[sizeof(Samples)];
    uint32_t value;
} Content;

struct RingbufferPayload {
    Topic topic;
    Timestamp timestamp;
    Content buffer;
};

#endif