// Copyright 2022-2023 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

// The sensors deliver 16 bit int coordinates. So X-Y fits in a 32 bit payload, which we use for events.
// We also model saturation here, using the extreme values of 16 bit ints.
// Sensors are responsible to provide their extremes this way.

#ifndef HEADER_INTCOORDINATE
#define HEADER_INTCOORDINATE

#include "Coordinate.h"
#include <climits>

union IntCoordinate {
    struct {
        int16_t x;
        int16_t y;
    };

    long l;

    bool operator==(const IntCoordinate& other) const {
        return x == other.x && y == other.y;
    }

    void set(const int16_t xIn, const int16_t yIn) {
        x = xIn;
        y = yIn;
    }

    double distanceFrom(const IntCoordinate other) const {
        return toCoordinate().distanceFrom(other.toCoordinate());
    }

    Coordinate toCoordinate() const {
        return { static_cast<double>(x), static_cast<double>(y) };
    }

    // We reserve SHRT_MIN to indicate saturated values
    // We can't use another field for quality, because we need to transport the coordinate in 32 bits.
    bool isSaturated() const {
        return x == SHRT_MIN || y == SHRT_MIN;
    }

    // SHRT_MAX indicates a read error (neither QMC nor HMC delivers this as a valid value)
    bool hasError() const {
        return x == SHRT_MAX || y == SHRT_MAX;
    }

    static IntCoordinate error() {
        return IntCoordinate { SHRT_MAX, SHRT_MAX };
    }
};

#endif
