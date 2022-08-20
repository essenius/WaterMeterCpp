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

#ifndef HEADER_COORDINATE
#define HEADER_COORDINATE

#include "ESP.h"

union Coordinate {
    struct {
        int16_t x;
        int16_t y;
    };

    long l;

    bool operator==(const Coordinate& other) const {
        return x == other.x && y == other.y;
    }

    void set(const int16_t xIn, const int16_t yIn) {
        x = xIn;
        y = yIn;
    }

    float distanceFrom(const Coordinate other) const {
        const auto xDifference = static_cast<float>(x - other.x);
        const auto yDifference = static_cast<float>(y - other.y);
        return sqrtf(xDifference * xDifference + yDifference * yDifference);
    }
};

#endif