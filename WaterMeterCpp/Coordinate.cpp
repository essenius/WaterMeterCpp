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

// ReSharper disable CppClangTidyClangDiagnosticFloatEqual - comparison is done to avoid division by 0.

#include <cstdio>
#include <cmath>
#include "Coordinate.h"

#include "IntCoordinate.h"
#include "MathUtils.h"

bool Coordinate::operator==(const Coordinate& other) const {
    printf("Coordinate== called\n");
    return aboutEqual(x, other.x) && aboutEqual(y, other.y);
}

// we need this to pass coordinates in a memory efficient way but still keep reasonable accuracy
IntCoordinate Coordinate::times10() const {
    return { {static_cast<int16_t>(x * 10),static_cast<int16_t>(y * 10)} };
}

Angle Coordinate::angle() const {
    if (x == 0 && y == 0) return {NAN};
    return {atan2(y, x)};
}

Angle Coordinate::angleFrom(const Coordinate& other) const {
    const Coordinate difference = translate(-other);
    return difference.angle();
}

double Coordinate::distance() const {
    return sqrt(x * x + y * y);
}

double Coordinate::distanceFrom(const Coordinate& other) const {
    const Coordinate difference = translate(-other);
    return difference.distance();
}

Coordinate Coordinate::rotate(const double angle) const {
    return {x * cos(angle) - y * sin(angle), y * cos(angle) + x * sin(angle)};
}

Coordinate Coordinate::translate(const Coordinate& vector) const {
    return {x + vector.x, y + vector.y};
}

Coordinate Coordinate::scale(const Coordinate& vector) const {
    return {x * vector.x, y * vector.y};
}

Coordinate Coordinate::reciprocal() const {
    return {1 / x, 1 / y};
}

Coordinate Coordinate::operator-() const {
    return {-x, -y};
}
