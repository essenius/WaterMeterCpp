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

#include "../WaterMeterCpp/IntCoordinate.h"

namespace WaterMeterCppTest {
	using EllipseMath::Coordinate;
	using WaterMeter::IntCoordinate;
	TEST(IntCoordinateTest, IntCoordinateTest1) {
		constexpr IntCoordinate Base{ {3, 4} };
		constexpr IntCoordinate Other{ {3, 4} };
		constexpr IntCoordinate BaseTimes10{ {30, 40} };
		ASSERT_TRUE(Base == Other) << "Equal works if equal";
		ASSERT_FALSE(Base == BaseTimes10) << "Equal works if not equal";
		const Coordinate baseAsCoordinate = Base.toCoordinate();
		const auto baseTimes10 = IntCoordinate::times10(baseAsCoordinate);
		ASSERT_TRUE(BaseTimes10 == baseTimes10) << "toCoordinate and times 10 conversion work";
		constexpr IntCoordinate Origin{ { 0, 0 } };
		ASSERT_EQ(0, Origin.l) << "Long casting works";
		ASSERT_EQ(5.0, Base.getDistanceFrom(Origin)) << "getDistance works";
		constexpr IntCoordinate SaturatedX{ { SHRT_MIN, 0 } };
		constexpr IntCoordinate SaturatedY{ { 0, SHRT_MIN } };

		ASSERT_TRUE(SaturatedX.isSaturated()) << "isSaturated works for X";
		ASSERT_TRUE(SaturatedY.isSaturated()) << "isSaturated works for Y";
		ASSERT_FALSE(Origin.isSaturated()) << "isSaturated works for non-saturated";

		auto error = IntCoordinate::error();
		ASSERT_TRUE(error.hasError()) << "error works";
		error.x = 0;
		ASSERT_TRUE(error.hasError()) << "error works if x is valid";
		error.y = 0;
		ASSERT_FALSE(error.hasError()) << "error works if x and y are valid";
	}
}