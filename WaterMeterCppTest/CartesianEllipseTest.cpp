#include <gtest/gtest.h>

#include "../WaterMeterCpp/MathUtils.h"
#include "TestHelper.h"
#include "../WaterMeterCpp/CartesianEllipse.h"

namespace WaterMeterCppTest {
	TEST(CartesianEllipseTest, RotatedTranslatedEllipseTest) {
		const auto quadraticEllipse = QuadraticEllipse(26, 20, 26, -772, -740, 7370);
		const auto cartesianEllipse = CartesianEllipse(quadraticEllipse);
		const auto center = cartesianEllipse.center;
		assertCoordinatesEqual(Coordinate{ 11,10 }, center, "Center");
		const auto radius = cartesianEllipse.radius;
		assertCoordinatesEqual(Coordinate{ 6, 4 }, radius, "Radius");
		const auto angle = cartesianEllipse.angle;
		assertAnglesEqual(Angle{ -M_PI / 4 }, angle, "Angle");
	}

	TEST(CartesianEllipseTest, PlainEllipseTest) {
		const auto quadraticEllipse = QuadraticEllipse(1, 0, 4, 0, 0, -4);
		const auto cartesianEllipse = CartesianEllipse(quadraticEllipse);
		const auto center = cartesianEllipse.center;
		assertCoordinatesEqual(Coordinate{ 0, 0 }, center, "Center");
		const auto radius = cartesianEllipse.radius;
		assertCoordinatesEqual(Coordinate{ 2, 1 }, radius, "Radius");
		const auto angle = cartesianEllipse.angle;
		assertAnglesEqual(Angle{ 0 }, angle, "Angle");
	}

	TEST(CartesianEllipseTest, TranslatedEllipseTest) {
		const auto quadraticEllipse = QuadraticEllipse(4, 0, 9, 40, 72, 208);
		const auto cartesianEllipse = CartesianEllipse(quadraticEllipse);
		const auto center = cartesianEllipse.center;
		assertCoordinatesEqual(Coordinate{ -5, -4 }, center, "Center");
		const auto radius = cartesianEllipse.radius;
		assertCoordinatesEqual(Coordinate{ 3, 2 }, radius, "Radius");
		const auto angle = cartesianEllipse.angle;
		assertAnglesEqual(Angle{ 0 }, angle, "Angle");
	}

	TEST(CartesianEllipseTest, RotatedEllipseTest) {
		const auto quadraticEllipse = QuadraticEllipse(4.94128, -3.90916, 8.05872, 0, 0, -36);
		const auto cartesianEllipse = CartesianEllipse(quadraticEllipse);
		const auto center = cartesianEllipse.center;
		assertCoordinatesEqual(Coordinate{ 0, 0 }, center, "Center");
		const auto radius = cartesianEllipse.radius;
		assertCoordinatesEqual(Coordinate{ 3, 2 }, radius, "Radius");
		const auto angle = cartesianEllipse.angle;
		assertAnglesEqual(Angle{ 0.4488 }, angle, "Angle");
	}

	TEST(CartesianEllipseTest, Rotated45EllipseTest) {
		const auto quadraticEllipse = QuadraticEllipse(90.5, -19, 90.5, 0, 0, -8100);
		const auto cartesianEllipse = CartesianEllipse(quadraticEllipse);
		const auto center = cartesianEllipse.center;
		assertCoordinatesEqual(Coordinate{ 0, 0 }, center, "Center");
		const auto radius = cartesianEllipse.radius;
		assertCoordinatesEqual(Coordinate{ 10, 9 }, radius, "Radius");
		const auto angle = cartesianEllipse.angle;
		assertAnglesEqual(Angle{ M_PI / 4 }, angle, "Angle");
	}

	TEST(CartesianEllipseTest, PointOnEllipseForTest) {
		const auto quadraticEllipse = QuadraticEllipse(106.75, 57.157677, 139.75, 7055.59545, 13777.81121, 369982.9419);
		const auto cartesianEllipse = CartesianEllipse(quadraticEllipse);
		assertAnglesEqual(Angle{ -M_PI / 6 }, cartesianEllipse.angle, "Angle");
		constexpr auto Origin = Coordinate{ 0, 0 };
		const auto originMap = cartesianEllipse.pointOnEllipseFor(Origin);
		assertCoordinatesEqual(Coordinate{ -16.9761709, -36.377509 }, originMap, "Origin mapped on ellipse");

		assertDoubleEqual(40.143662, cartesianEllipse.getDistanceFrom(Origin), "Distance from origin");
		constexpr auto PointInEllipse = Coordinate{ -25, -40 };
		const auto pointInEllpseMap = cartesianEllipse.pointOnEllipseFor(PointInEllipse);
		assertCoordinatesEqual(Coordinate{ -28.455996, -35.680005 }, pointInEllpseMap, "Point inside ellipse mapped on ellipse");
		assertDoubleEqual(5.532293, cartesianEllipse.getDistanceFrom(PointInEllipse), "Distance from point inside ellipse");
	}

	TEST(CartesianEllipseTest, CircleCircumferenceTest) {
		// circle with a diameter of 100 - we can calculate the exact circumference
		const auto quadraticCircle = QuadraticEllipse(100, 0, 100, 0, 0, -10000);
		const auto cartesianCircle = CartesianEllipse(quadraticCircle);
		assertDoubleEqual(2 * M_PI * cartesianCircle.radius.x, cartesianCircle.circumference(), "Circumference");
		// now we're at it, see if getAngle doesn't crash and burn
		const auto angle = cartesianCircle.angle;
		assertAnglesEqual(Angle{ M_PI / 2 }, angle, "Circle getAngle");
	}

	TEST(CartesianEllipseTest, EllipseCircumferenceTest) {
		const auto quadraticCircle = QuadraticEllipse(100, 0, 121, 0, 0, -12100);
		const auto cartesianCircle = CartesianEllipse(quadraticCircle);
		EXPECT_TRUE(cartesianCircle.fitSucceeded()) << "Fit succeeded";
		assertDoubleEqual(66.01085, cartesianCircle.circumference(), "Circumference");
	}

	TEST(CartesianEllipseTest, NoFitTest) {
		const auto quadraticCircle = QuadraticEllipse(0, 0, 0, 0, 0, 0);
		const auto cartesianCircle = CartesianEllipse(quadraticCircle);
		EXPECT_FALSE(cartesianCircle.fitSucceeded()) << "Fit not succeeded";
	}
}
