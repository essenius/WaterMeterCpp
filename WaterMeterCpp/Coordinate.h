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

// Various calculations for coordinates, which are the basis for more complex calcuations in e.g. ellipses

#ifndef COORDINATE_H
#define COORDINATE_H
#include "Angle.h"

struct Coordinate {
	double x;
	double y;

    bool operator==(const Coordinate& other) const;
	Angle getAngle() const;
	Angle getAngleFrom(const Coordinate& other) const;
	double getDistance() const;
	double getDistanceFrom(const Coordinate& other) const;
	Coordinate operator-() const;
	Coordinate getReciprocal() const;
	Coordinate rotated(double angle) const;
	Coordinate translated(const Coordinate& vector) const;
	Coordinate scaled(const Coordinate& vector) const;
};
#endif
