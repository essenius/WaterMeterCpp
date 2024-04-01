// Copyright 2023-2024 Rik Essenius
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

#include "SensorSample.h"

namespace WaterMeterCppTest {
	using EllipseMath::Coordinate;
	using WaterMeter::SensorSample;
	using WaterMeter::SensorState;
	TEST(SensorSampleTest, scriptTest) {
		constexpr SensorSample Base{ {3, 4} };
		constexpr SensorSample Other{ {3, 4} };
		constexpr SensorSample BaseTimes10{ {30, 40} };
		ASSERT_TRUE(Base == Other) << "Equal works if equal";
		ASSERT_FALSE(Base == BaseTimes10) << "Equal works if not equal";
		const Coordinate baseAsCoordinate = Base.toCoordinate();
		const auto baseTimes10 = SensorSample::times10(baseAsCoordinate);
		ASSERT_TRUE(BaseTimes10 == baseTimes10) << "toCoordinate and times 10 conversion work";
		constexpr SensorSample Origin{ { 0, 0 } };
		ASSERT_EQ(0, Origin.l) << "Long casting works";
		ASSERT_EQ(5.0, Base.getDistanceFrom(Origin)) << "getDistance works";
		constexpr SensorSample SaturatedX{ { SHRT_MIN, 0 } };
		constexpr SensorSample SaturatedY{ { 0, SHRT_MIN } };

		ASSERT_TRUE(SaturatedX.isSaturated()) << "isSaturated works for X";
		ASSERT_TRUE(SaturatedY.isSaturated()) << "isSaturated works for Y";
		ASSERT_FALSE(Origin.isSaturated()) << "isSaturated works for non-saturated";

		auto error = SensorSample::error(SensorState::ReadError);
		ASSERT_EQ(SensorState::ReadError, error.state()) << "error works";
        const auto oldX = error.x;
		error.x = 0;
		ASSERT_EQ(SensorState::Ok, error.state()) << "no more error if x is valid";
		error.x = oldX;
		error.y = 0;
		ASSERT_EQ(SensorState::None, error.state()) << "can set Y to force a certain error";
	}

	TEST(SensorSampleTest, toStringTest) {
		// for each value in the enum, check if the string is correct
		ASSERT_STREQ("None", SensorSample::stateToString(SensorState::None)) << "None";
		ASSERT_STREQ("Ok", SensorSample::stateToString(SensorState::Ok)) << "Ok";
		ASSERT_STREQ("PowerError", SensorSample::stateToString(SensorState::PowerError)) << "PowerError";
		ASSERT_STREQ("BeginError", SensorSample::stateToString(SensorState::BeginError)) << "BeginError";
		ASSERT_STREQ("ReadError", SensorSample::stateToString(SensorState::ReadError)) << "ReadError";
		ASSERT_STREQ("Saturated", SensorSample::stateToString(SensorState::Saturated)) << "Saturated";
		ASSERT_STREQ("NeedsHardReset", SensorSample::stateToString(SensorState::NeedsHardReset)) << "NeedsHardReset";
		ASSERT_STREQ("NeedsSoftReset", SensorSample::stateToString(SensorState::NeedsSoftReset)) << "NeedsSoftReset";
		ASSERT_STREQ("Resetting", SensorSample::stateToString(SensorState::Resetting)) << "Resetting";
		ASSERT_STREQ("FlatLine", SensorSample::stateToString(SensorState::FlatLine)) << "FlatLine";
		ASSERT_STREQ("Outlier", SensorSample::stateToString(SensorState::Outlier)) << "Outlier";
		ASSERT_STREQ("Unknown", SensorSample::stateToString(static_cast<SensorState>(-1))) << "Unknown";
	}
}