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

#ifndef HEADER_ANGLE
#define HEADER_ANGLE
#include "FloatCoordinate.h"

enum SearchTarget : uint8_t {
    None = 0,
    MaxY,
    MaxX,
    MinY,
    MinX
};

class Angle {
public:
	static constexpr float PI_F = 3.1415926536f;
	static constexpr float MIN_RELATIVE_ANGLE = -0.25;
	static constexpr float MAX_RELATIVE_ANGLE = 0.25;
    static constexpr int DEFAULT_ANGLE_COUNT = 1;

    Angle();
    bool isAcceptable(FloatCoordinate to);
    SearchTarget firstTarget() const;
    void setFrom(FloatCoordinate from);
    void setTarget(SearchTarget target);
    float value() const { return _value; }
private:
    void countToNextMeasurement(float difference);
    float differenceWith(float previousAngle) const;
    static float startAngleFromExtreme(SearchTarget target);

    float _value;
    FloatCoordinate _from;
    int _countdown = 0;
    float _maxAngle = PI_F;
    float _minAngle = -PI_F;
};
#endif
