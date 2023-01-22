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

#include "TestHelper.h"
#include "../WaterMeterCpp/Angle.h"

TEST(AngleTest, QuadrantTest) {
	constexpr double ANGLE_EPSILON = 0.001;
	auto angle = Angle{ ANGLE_EPSILON };
    EXPECT_EQ(1, angle.quadrant());
	angle.value = M_PI / 2 - ANGLE_EPSILON;
	EXPECT_EQ(1, angle.quadrant());

	angle.value = M_PI - ANGLE_EPSILON;
	EXPECT_EQ(2, angle.quadrant());
	angle.value = M_PI / 2 + ANGLE_EPSILON;
	EXPECT_EQ(2, angle.quadrant());

	angle.value = -M_PI + ANGLE_EPSILON;
	EXPECT_EQ(3, angle.quadrant());
	angle.value = -M_PI / 2 - ANGLE_EPSILON;
	EXPECT_EQ(3, angle.quadrant());

	angle.value = -ANGLE_EPSILON;
	EXPECT_EQ(4, angle.quadrant());
	angle.value = -M_PI / 2 + ANGLE_EPSILON;
	EXPECT_EQ(4, angle.quadrant());
}

TEST(AngleTest, CompareTest) {
	constexpr Angle A {M_PI};
	constexpr Angle B {M_PI};
	assertAnglesEqual(A, { M_PI }, "a = M_PI");
	assertAnglesEqual(A, B, "a=b");
	assertAnglesEqual({ M_PI }, { M_PI }, "M_PI = M_PI");
}

TEST(AngleTest, OperatorTest) {
	constexpr Angle A {M_PI};
	const auto b = A - M_PI / 3;
	assertDoubleEqual(2 * M_PI / 3, b, "operator- on double");
	const auto c = A - Angle{ M_PI / 3 };
	assertAnglesEqual({ 2 * M_PI / 3 }, c, "operator- on angle");
}