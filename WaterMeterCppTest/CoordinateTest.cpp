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

#include "../WaterMeterCpp/Coordinate.h"

namespace WaterMeterCppTest {

	bool coordinateEqual(const Coordinate& a, const Coordinate& b) {
		return a == b;
	}

	TEST(CoordinateTest, CoordinateTest1) {
		constexpr Coordinate Coordinate1{ 4, 4 };
		ASSERT_DOUBLE_EQ(M_PI / 4, Coordinate1.getAngle().value) << "Angle OK";
		ASSERT_DOUBLE_EQ(sqrt(32), Coordinate1.getDistance()) << "Distance OK";
		ASSERT_TRUE(coordinateEqual(Coordinate1, Coordinate1)) << "Equal to itself";
		ASSERT_TRUE(coordinateEqual(Coordinate{ 4,4 }, Coordinate1)) << "Equal to a coordinate with the same values";
		constexpr Coordinate Coordinate2{ 0, 7 };
		ASSERT_FALSE(coordinateEqual(Coordinate2, Coordinate1)) << "Coordinates 1 and 2 not equal";
		ASSERT_FALSE(coordinateEqual(Coordinate{ 4, -4 }, Coordinate1)) << "Not equal to a coordinate with a different Y value";
		ASSERT_DOUBLE_EQ(5, Coordinate1.getDistanceFrom(Coordinate2)) << "DistanceFrom OK";
		constexpr Coordinate Coordinate3{ -4, 12 };
		ASSERT_DOUBLE_EQ(-M_PI / 4, Coordinate1.getAngleFrom(Coordinate3).value) << "AngleFrom OK";
		const auto rotated = Coordinate1.rotated(-M_PI / 2);
		ASSERT_TRUE(coordinateEqual(Coordinate{ 4, -4 }, rotated)) << "Rotated OK";
		const auto translated = Coordinate1.translated(Coordinate3);
		ASSERT_TRUE(coordinateEqual(Coordinate{ 0, 16 }, translated)) << "Translate OK";
		const auto scaled = Coordinate1.scaled({ 0.125, 2 });
		ASSERT_TRUE(coordinateEqual(Coordinate{ 0.5, 8 }, scaled)) << "Scaled OK";
		const auto reciproke = scaled.getReciprocal();
		ASSERT_TRUE(coordinateEqual(Coordinate{ 2, 0.125 }, reciproke)) << "Reciproke OK";

		ASSERT_TRUE(isnan(Coordinate{ 0, 0 }.getAngle().value)) << "Angle of 0,0 is not defined";

	}
}