// Copyright 2023 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#ifndef CARTESIAN_ELLIPSE_H
#define CARTESIAN_ELLIPSE_H

#include "Angle.h"
#include "QuadraticEllipse.h"
#include "Coordinate.h"

struct CartesianEllipse {
	QuadraticEllipse coefficient {};
	Coordinate center {};
	Coordinate radius {};
	Angle angle {};
	bool hasData = false;

	CartesianEllipse() = default;
	CartesianEllipse(const Coordinate& center, const Coordinate& radius, const Angle& angle);
	explicit CartesianEllipse(const QuadraticEllipse& quadraticEllipse);

	double circumference() const;
	double distanceFrom(const Coordinate& referencePoint) const;
	Coordinate parametricRepresentation(const Angle& referenceAngle) const;
	Coordinate pointOnEllipseFor(const Coordinate& referencePoint) const;
};
#endif