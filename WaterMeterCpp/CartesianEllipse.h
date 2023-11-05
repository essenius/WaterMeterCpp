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

// CartesianEllipse supports the following calculations based on a (fitted) QuadraticEllipse
// * the getDistance between a point an the ellipse
// * the coordinates of a point projected on the ellipse
// * the coordinate of the parametric representation of the ellipse given an getAngle
// * an approximation of the circumference of the circle
//
// These methods are used to determine whether we have an outlier: too far away from the ellipse => outlier

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

	bool fitSucceeded() const;
	double circumference() const;
	double getDistanceFrom(const Coordinate& referencePoint) const;
	Coordinate parametricRepresentation(const Angle& referenceAngle) const;
	Coordinate pointOnEllipseFor(const Coordinate& referencePoint) const;
};
#endif