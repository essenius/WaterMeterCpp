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

#include <gtest/gtest.h>
#include <corecrt_math_defines.h>
#include <cstdio>
#include "TestHelper.h"

// interesting: https://cookierobotics.com/007/



#include "../WaterMeterCpp/QuadraticEllipse.h"
#include "../WaterMeterCpp/MathUtils.h"

void assertEllipse(const Coordinate& center, const Coordinate& radius, const Angle& angle) {
	const double a = sqr(radius.x) * sqr(angle.sin()) + sqr(radius.y) * sqr(angle.cos());
	const double b = 2 * (sqr(radius.y) - sqr(radius.x)) * angle.sin() * angle.cos();
	const double c = sqr(radius.x) * sqr(angle.cos()) + sqr(radius.y) * sqr(angle.sin());
	const double d = -2 * a * center.x - b * center.y;
	const double e = -b * center.x - 2 * c * center.y;
	const double f = a * sqr(center.x) + b * center.x * center.y + c * sqr(center.y) - sqr(radius.x) * sqr(radius.y);

	QuadraticEllipse coefficient(a, b, c, d, e, f);
	const auto center1 = coefficient.center();
	assertCoordinatesEqual(center, center1, "Center");
	const auto radius1 = coefficient.radius();
	assertCoordinatesEqual(radius, radius1, "Radius");
	assertAnglesEqual(angle, coefficient.angle(), "Angle");
}

TEST(QuadraticEllipseTest, RoundTripTest) {
	assertEllipse(Coordinate{ 0, 0 }, Coordinate{ 2, 1 }, Angle{0});
	assertEllipse(Coordinate{ 10, 10 }, Coordinate{ 2, 1 }, Angle{0});
	assertEllipse(Coordinate{ 10, 10 }, Coordinate{ 2, 1 }, Angle{M_PI / 3});
	// measured ellipse
	assertEllipse(Coordinate{ -29, -55 }, Coordinate{ 12, 9 }, Angle{-0.51});
	assertEllipse(Coordinate{ -21.09, -45.71 }, Coordinate{ 12.68, 9.88 }, Angle{1.09});
	assertEllipse(Coordinate{ 10, 11 }, Coordinate{ 6, 4 }, Angle{-M_PI / 6});
}
