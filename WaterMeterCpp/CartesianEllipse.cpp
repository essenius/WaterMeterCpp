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
	center = coefficient.center();
	radius = coefficient.radius();
	angle = coefficient.angle();
	hasData = true;
}

double CartesianEllipse::circumference() const {

	// approximation, not so easy to determine precisely.
	// See https://www.johndcook.com/blog/2013/05/05/ramanujan-circumference-ellipse/
    const auto t = 3 * sqr((radius.x - radius.y) / (radius.x + radius.y));
	return M_PI * (radius.x + radius.y) * (1 + t / (10 + sqrt(4 - t)));

}

Coordinate CartesianEllipse::parametricRepresentation(const Angle& referenceAngle) const {
	// Note the angle is relative to the center of the ellipse, not the origin 
	return {
		center.x + radius.x * cos(referenceAngle.value) * cos(angle.value) -
		radius.y * sin(referenceAngle.value) * sin(angle.value),
		center.y + radius.y * sin(referenceAngle.value) * cos(angle.value) +
		radius.x * cos(referenceAngle.value) * sin(angle.value)
	};
}

Coordinate CartesianEllipse::pointOnEllipseFor(const Coordinate& referencePoint) const {
	// Normalize the point, then find the angle with the origin. This gives the angle that parametricRepresentation needs.
	const auto transformedCoordinate = referencePoint
		.translate(-center)
		.rotate(-angle.value)
		.scale(radius.reciprocal());
	const auto angleWithOrigin = transformedCoordinate.angle();
	return parametricRepresentation(angleWithOrigin);
}

double CartesianEllipse::distanceFrom(const Coordinate& referencePoint) const {
	return pointOnEllipseFor(referencePoint).distanceFrom(referencePoint);
}
