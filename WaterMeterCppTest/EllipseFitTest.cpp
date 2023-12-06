#include <gtest/gtest.h>
#include <corecrt_math_defines.h>

#include "TestHelper.h"
#include "../WaterMeterCpp/QuadraticEllipse.h"
#include "../WaterMeterCpp/CartesianEllipse.h"
#include "../WaterMeterCpp/EllipseFit.h"

namespace WaterMeterCppTest {

	void assertPerfectEllipse(const Coordinate& center, const Coordinate& radius, const Angle& angle) {
		const auto inputEllipse = CartesianEllipse(center, radius, angle);
		EllipseFit ellipseFit;
		const unsigned int pointsOnEllipse = EllipseFit::getSize();
		ellipseFit.begin();
		auto x = -M_PI;
		for (unsigned int i = 0; i < pointsOnEllipse; i++) {
			auto point = inputEllipse.getPointOnEllipseAtAngle(Angle{ x });
			ASSERT_TRUE(ellipseFit.addMeasurement(point)) << "Point " << x << " added";
			x += M_PI / (pointsOnEllipse / 2.0);
		}
		ASSERT_FALSE(ellipseFit.addMeasurement({ 0,0 })) << "Unable to add more points";

		const auto resultQuadraticEllipse = ellipseFit.fit();
		const auto resultEllipse = CartesianEllipse(resultQuadraticEllipse);
		printf("Input:\n");
		printf("\nResult:\n");
		printf("\n");

		assertCoordinatesEqual(center, resultEllipse.center, "Center", Epsilon);
		assertCoordinatesEqual(radius, resultEllipse.radius, "Radius", Epsilon);
		if (fabs(radius.x - radius.y) > Epsilon) assertAnglesEqual(angle, resultEllipse.angle, "Angle", Epsilon);

		// check if all points of the parametric representation of the result ellipse are on the original ellipse
		x = -M_PI;
		for (int i = -16; i < static_cast<int>(pointsOnEllipse); i++) {
			Coordinate pointOut = resultEllipse.getPointOnEllipseAtAngle(Angle{ x });
			assertDoubleEqual(0, inputEllipse.getDistanceFrom(pointOut), "Distance", 0.0001);
			x += M_PI / (pointsOnEllipse / 2.0);
		}
	}

	TEST(EllipseFitTest, PerfectFitTestTest) {

		// A point does not work. Radii need to be > 0. If 0, result will be -nan(ind).

		// Circle with center = (100, -100), radius=(8,8), angle = irrelevant
		assertPerfectEllipse({ 100, -100 }, { 8, 8 }, { 0 });

		// ellipse with center = (1,3), radius = (12,10), angle = pi/4, i.e. quite round
		assertPerfectEllipse({ 1, 3 }, { 12, 10 }, { M_PI / 4 });

		// ellipse with center = (0,0), radius = (20,1), angle = pi/3, i.e. very flat ellipse
		assertPerfectEllipse({ 0, 0 }, { 20, 1 }, { M_PI / 3 });

		// ellipse with center = (-21.09,-45.71), radius = (12.68,9.88), angle = -0.48; close to one measured
		assertPerfectEllipse({ -21.09, -45.71 }, { 12.68, 9.88 }, { -0.48 });
	}


	// create two ellipses at a distance from the expected result, and add points from each to the fitter.

	void assertEllipseWithDistance(const Coordinate& center, const Coordinate& radius, const Angle& angle, const double& distance) {
		auto distancec = Coordinate{ distance, distance };
		const auto innerEllipse = CartesianEllipse(center, radius.translated(-distancec), angle);
		const auto outerEllipse = CartesianEllipse(center, radius.translated(distancec), angle);
		const unsigned int pointsOnEllipse = EllipseFit::getSize();
		EllipseFit ellipseFit;
		ellipseFit.begin();
		auto x = -M_PI;
		for (unsigned int i = 0; i < pointsOnEllipse; i++) {
			Coordinate point = i % 2 == 0
				? innerEllipse.getPointOnEllipseAtAngle(Angle{ x })
				: outerEllipse.getPointOnEllipseAtAngle(Angle{ x });
			ASSERT_TRUE(ellipseFit.addMeasurement(point)) << "Point " << x << " added";
			x += M_PI / (pointsOnEllipse / 2.0);
		}
		ASSERT_FALSE(ellipseFit.addMeasurement({ 0,0 })) << "Unable to add more points";

		const auto resultQuadraticEllipse = ellipseFit.fit();
		const auto resultEllipse = CartesianEllipse(resultQuadraticEllipse);

		x = -M_PI;
		assertCoordinatesEqual(center, resultEllipse.center, "Center", Epsilon);
		assertCoordinatesEqual(radius, resultEllipse.radius, "Radius", 0.01);
		if (fabs(radius.x - radius.y) > Epsilon) assertAnglesEqual(angle, resultEllipse.angle, "Angle", Epsilon);

		const auto middleEllipse = CartesianEllipse(center, radius, angle);

		for (int i = -16; i < static_cast<int>(pointsOnEllipse); i++) {
			Coordinate pointOut = resultEllipse.getPointOnEllipseAtAngle(Angle{ x });
			assertDoubleEqual(0, middleEllipse.getDistanceFrom(pointOut), "Distance", 0.01);
			x += M_PI / (pointsOnEllipse / 2.0);
		}
	}

	TEST(EllipseFitTest, FitWithDistanceTest) {
		assertEllipseWithDistance({ 0,0 }, { 10, 5 }, { -M_PI / 6 }, 0.1);
		assertEllipseWithDistance({ -30,-20 }, { 12, 9 }, { M_PI / 5 }, 0.1);
	}

	void assertPartialEllipse(const CartesianEllipse& ellipse, const double& fraction, const double& startAngle) {

		const unsigned int points = EllipseFit::getSize();
		EllipseFit ellipseFit;
		ellipseFit.begin();
		const double delta = M_PI * 2.0 * fraction / points;
		auto x = startAngle;
		for (unsigned int i = 0; i < points; i++) {
			Coordinate point = ellipse.getPointOnEllipseAtAngle(Angle{ x });
			ASSERT_TRUE(ellipseFit.addMeasurement(point)) << "Point " << x << " added";
			printf(" | ");
			x += delta;
		}
		ASSERT_FALSE(ellipseFit.addMeasurement({ 0,0 })) << "Unable to add more points";

		const auto resultQuadraticEllipse = ellipseFit.fit();
		const auto resultEllipse = CartesianEllipse(resultQuadraticEllipse);

		// we need to accept a bit more variation here 
		constexpr double Epsilon1 = 0.0005;
		assertCoordinatesEqual(ellipse.center, resultEllipse.center, "Center", Epsilon1);
		assertCoordinatesEqual(ellipse.radius, resultEllipse.radius, "Radius", Epsilon1);
		if (fabs(ellipse.radius.x - ellipse.radius.y) > Epsilon) assertAnglesEqual(ellipse.angle, resultEllipse.angle, "Angle", Epsilon1);

		x = -M_PI;
		constexpr int PointsOnEllipse = 32;
		for (int i = -16; i < PointsOnEllipse; i++) {
			Coordinate pointOut = resultEllipse.getPointOnEllipseAtAngle(Angle{ x });
			assertDoubleEqual(0, ellipse.getDistanceFrom(pointOut), "Distance", 0.001);
			x += M_PI / (PointsOnEllipse / 2.0);
		}
	}

	TEST(EllipseFitTest, PartialEllipseTest) {
		// take 20% of an ellipse, starting at M_PI/2
		assertPartialEllipse(CartesianEllipse({ 20,-20 }, { 10, 4 }, { M_PI / 5 }), 0.2, M_PI / 2);
	}

	TEST(EllipseFitTest, StableFitTest) {
		EllipseFit ellipseFit;
		ASSERT_EQ(32, ellipseFit.getSize()) << "Size OK";
		ASSERT_EQ(0, ellipseFit.getPointCount()) << "PointCount OK";
		ellipseFit.addMeasurement({ 1,2 });
		ASSERT_EQ(32, ellipseFit.getSize()) << "Size OK";
		ASSERT_EQ(1, ellipseFit.getPointCount()) << "PointCount OK";
	}
}
