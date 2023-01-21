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

#include "ExtremeSearcher.h"

void ExtremeSearcher::begin(const float maxNoiseDistance) {
    _maxNoiseDistance = maxNoiseDistance;
    _wasFound = false;
    _extreme = _initValue;
    _angle.setTarget(_target);
}

bool ExtremeSearcher::isExtreme(const FloatCoordinate sample) const {
    switch(_target) {
    case MaxY:
        return sample.y > _extreme.y;
    case MaxX:
        return sample.x > _extreme.x;
    case MinY:
        return sample.y < _extreme.y;
    case MinX:
        return sample.x < _extreme.x;
    default: // includes None, only set the first time
        return fabsf(_extreme.x - MIN_SENSOR_VALUE) <= 0.001f;
    }
}

void ExtremeSearcher::addMeasurement(const FloatCoordinate sample) {
    if (isExtreme(sample)) {
        _extreme = sample;
        _angle.setFrom(_extreme);
    }
    // need to calculate both all the time, since they use previous values (no boolean shortcuts)
    const auto distance = _extreme.distanceFrom(sample);
    const auto angleOk = _angle.isAcceptable(sample);
    _wasFound = distance > _maxNoiseDistance && angleOk;
}

bool ExtremeSearcher::foundTarget() const {
    return _wasFound;
}

ExtremeSearcher* ExtremeSearcher::next() const {
    _nextSearcher->begin(_maxNoiseDistance);
    return _nextSearcher;
}

FloatCoordinate ExtremeSearcher::extreme() const {
    return _extreme;
}

SearchTarget ExtremeSearcher::target() const {
    if (_target == None) return _angle.firstTarget();
    return _target; 
}
