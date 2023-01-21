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

#ifndef HEADER_EXTREMESEARCHER
#define HEADER_EXTREMESEARCHER

#include "Angle.h"
#include "FloatCoordinate.h"

// Don't use this class with the None target as its behavior is not defined for that 

class ExtremeSearcher {
public:
    static constexpr float MIN_SENSOR_VALUE = -4096;
    static constexpr float MAX_SENSOR_VALUE = 4096;
    ExtremeSearcher(const SearchTarget target, const FloatCoordinate initValue, ExtremeSearcher* next) :
    _target(target), _initValue(initValue), _extreme(initValue), _nextSearcher(next) {}

    void begin(float maxNoiseDistance);
    bool isExtreme(FloatCoordinate sample) const;
    void addMeasurement(FloatCoordinate sample);
    bool foundTarget() const;
    ExtremeSearcher* next() const;
    FloatCoordinate extreme() const;
    SearchTarget target() const;

private:
    static constexpr int DEFAULT_ANGLE_COUNT = 5;
    // what extreme are we looking for?
    SearchTarget _target;
    // could we confirm we found it?
    bool _wasFound = false;
    // Values that must be immediately overridden with the first comparison,
    // i.e. the lowest value the sensor can return for a max, and the highest for a min.
    FloatCoordinate _initValue;
    // the current extreme
    FloatCoordinate _extreme;
    // which extreme will we search for after this one?
    ExtremeSearcher* _nextSearcher;
    // The maximum amount of noise we can expect (set with begin())
    float _maxNoiseDistance = 0;
    Angle _angle;
};

#endif