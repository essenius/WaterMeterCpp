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

#include <cmath>
#include <cstdio>
#include "CartesianEllipse.h"
#include "MathUtils.h"

CartesianEllipse::CartesianEllipse(const Coordinate& center, const Coordinate& radius, const Angle& angle) {
	this->center = center;
	this->radius = radius;
	this->angle = angle;
	hasData = true;
}

CartesianEllipse::CartesianEllipse(const QuadraticEllipse& quadraticEllipse) {
	coefficient = quadraticEllipse;
	center = coefficient.getCenter();
	radius = coefficient.getRadius();
	angle = coefficient.getAngle();
	hasData = true;
}

bool CartesianEllipse::isValid() const {
	return radius.getDistance() > Epsilon;
}

double CartesianEllipse::getCircumference() const {
	// approximation, not so easy to determine precisely.
	// See https://www.johndcook.com/blog/2013/05/05/ramanujan-getCircumference-ellipse/
    const auto t = 3 * sqr((radius.x - radius.y) / (radius.x + radius.y));
	return M_PI * (radius.x + radius.y) * (1 + t / (10 + sqrt(4 - t)));

}

// Calculates the parametric representation of the ellipse at the given angle
// Note the angle is relative to the center of the ellipse, not the origin 
Coordinate CartesianEllipse::getPointOnEllipseAtAngle(const Angle& referenceAngle) const {
	return Coordinate {
		center.x + radius.x * cos(referenceAngle.value) * cos(angle.value) -
		radius.y * sin(referenceAngle.value) * sin(angle.value),
		center.y + radius.y * sin(referenceAngle.value) * cos(angle.value) +
		radius.x * cos(referenceAngle.value) * sin(angle.value)
	};
}

// This function takes a reference point in the Cartesian coordinate system and returns the point on the ellipse that is closest to the reference point.
// The reference point is first translated by the center of the ellipse, then rotated by the angle of the ellipse, then scaled by the reciprocal of the radius.
// The resulting point is then used as the parametric input to the parametric representation of the ellipse.
// The output of the parametric representation is the point on the ellipse that is closest to the reference point.
Coordinate CartesianEllipse::getPointOnEllipseClosestTo(const Coordinate& referencePoint) const {
	// Normalize the point, then find the angle with the origin. This gives the angle that getPointOnEllipseAtAngle needs.
	const auto transformedCoordinate = referencePoint
		.translated(-center)
		.rotated(-angle.value)
		.scaled(radius.getReciprocal());
	const auto angleWithOrigin = transformedCoordinate.getAngle();
	return getPointOnEllipseAtAngle(angleWithOrigin);
}

// returns the distance from the given point to the closest point on the ellipse
double CartesianEllipse::getDistanceFrom(const Coordinate& referencePoint) const {
	return getPointOnEllipseClosestTo(referencePoint).getDistanceFrom(referencePoint);
}
