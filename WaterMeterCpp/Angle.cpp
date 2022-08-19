// Copyright 2022 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#include "Angle.h"
#include <cmath>

Angle::Angle() : _from({0,0}) {
    _value = 0.0f;
}

bool Angle::isAcceptable(const FloatCoordinate to) {
    // with 0 distance we would get an error on atan2, so keep the previous value
    if (fabsf(to.distanceFrom(_from)) > 0) {
        const auto previousDirection = _value;
        _value = atan2f(to.y - _from.y, to.x - _from.x);
        const float difference = differenceWith(previousDirection);
        countToNextMeasurement(difference);
    }
    return _countdown == 0;
}

// based on the direction we are going, determine the next extreme we will see
SearchTarget Angle::firstTarget() const {
    if (_value < -PI_F / 2) return MinY;
    if (_value < 0.0f) return MaxX;
    if (_value < PI_F / 2) return MaxY;
    return MinX;
}

void Angle::setFrom(const FloatCoordinate from) {
    _from = from;
}

void Angle::setTarget(const SearchTarget target) {
    _maxAngle = startAngleFromExtreme(target);
    // the minumum angle is 45 degrees less. Not entirely true with an ellipse,
    // but since the ellipse is not very flat it is close enough.
    _minAngle = target == None ? -PI_F : _maxAngle - PI_F / 4;
}

// ---- Private methods ----

// If we have large angle deltas or are are outside the expected range, there is most likely no flow (noisy angle data)
// Wait a couple of samples before trying again. This reduces the risk of accidental right angle data.

void Angle::countToNextMeasurement(const float difference) {
    if (difference < MIN_RELATIVE_ANGLE || difference > MAX_RELATIVE_ANGLE ||
        _value < _minAngle || _value > _maxAngle) {
        _countdown = DEFAULT_ANGLE_COUNT;
        return;
    }
    if (_countdown == 0) return;
    _countdown -= 1;
}

float Angle::differenceWith(const float previousAngle) const {
    const auto difference = _value - previousAngle;
    // the atan2 function returns values between -PI and PI.
    // If there is noise around the extreme values the difference flips around, giving values of around 2*PI.
    // We correct that to have values between -PI and PI again.
    if (difference > PI_F) return difference - 2 * PI_F;
    if (difference < -PI_F) return difference + 2 * PI_F;
    return difference;
}

// this is the maximum angle we will see just after passing the extreme
float Angle::startAngleFromExtreme(const SearchTarget target) {
    switch (target) {
    case MaxX: return -PI_F / 2;
    case MinY: return PI_F;
    case MinX: return PI_F / 2;
    case MaxY: return 0;
    default: // includes None, don't limit
        return PI_F;
    }
}
