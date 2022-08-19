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

#ifndef HEADER_FLOATCOORDINATE
#define HEADER_FLOATCOORDINATE

#include "Coodinate.h"

struct FloatCoordinate {
    float x;
    float y;

    void set(const Coordinate input) {
        x = static_cast<float>(input.x);
        y = static_cast<float>(input.y);
    }

    float distanceFrom(const FloatCoordinate other) const {
        const auto difference = differenceWith(other);
        return sqrtf(difference.x * difference.x + difference.y * difference.y);
    }

    FloatCoordinate differenceWith(const FloatCoordinate other) const {
        return { x - other.x, y - other.y };
    }
};

#endif