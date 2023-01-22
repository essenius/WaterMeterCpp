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
#include "MathUtils.h"
#include "QuadraticEllipse.h"
#include <cstdio>

// we use the base formula from https://mathworld.wolfram.com/CartesianEllipse.html: ax^2 + 2bxy + cy^2 + 2dx + 2fy + g = 0
// we expect the constructor to deliver based on ax^2 + bxy + cy^2 + dx + fy + g = 0
// See also https://scipython.com/blog/direct-linear-least-squares-fitting-of-an-ellipse/

// we use a, b, c, d, f, g as Wolfram does that too - probably to not use e, which can cause confusion with the mathematical number

QuadraticEllipse::QuadraticEllipse(const double& a1, const double& b1, const double& c1, const double& d1, const double& f1, const double& g1) {
	a = a1;
	b = b1 / 2;
	c = c1;
	d = d1 / 2;
	f = f1 / 2;
	g = g1;
	_switchedAxes = false;
	_radiusCalculated = false;
}

Angle QuadraticEllipse::angle() {
	if (b == 0) {  // NOLINT(clang-diagnostic-float-equal) -- avoiding division by 0
		return { (a < c ? 0 : M_PI / 2) };
	}

	// we need the radius calculated, since that can cause 
	if (!_radiusCalculated) radius();

	auto baseAngle = 0.5 * atan2(2 * b, a - c) + M_PI / 2;
	if (_switchedAxes) {
		baseAngle += M_PI / 2;
	}
	if (baseAngle > M_PI / 2) baseAngle -= M_PI;
	return { baseAngle };
}

Coordinate QuadraticEllipse::center() const {
	return { (c * d - b * f) / discriminant(), (a * f - b * d) / discriminant() };
}

double QuadraticEllipse::discriminant() const {
	return sqr(b) - a * c;
}

Coordinate QuadraticEllipse::radius() {
	if (_radiusCalculated) return _radius;

	const double numerator = 2 * (a * sqr(f) + c * sqr(d) + g * sqr(b) - 2 * b * d * f - a * c * g);
	const double partialDenominator = sqrt(sqr(a - c) + 4 * sqr(b));
	const double widthDenominator = discriminant() * (partialDenominator - (a + c));
	const double heightDenominator = discriminant() * (-partialDenominator - (a + c));

	_radius.x = sqrt(numerator / widthDenominator);
	_radius.y = sqrt(numerator / heightDenominator);
	if (_radius.x < _radius.y) {
		const auto temp = _radius.x;
		_radius.x = _radius.y;
		_radius.y = temp;
		_switchedAxes = true;
	}
	_radiusCalculated = true;
	return _radius;
}

void QuadraticEllipse::print() const {
	printf("a=%.3f, b=%.3f, c=%.3f, d=%.3f, f=%.3f, g=%.3f", a, b, c, d, f, g);
}
