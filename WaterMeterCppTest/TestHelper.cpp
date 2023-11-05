#include <gtest/gtest.h>
#include "TestHelper.h"

#include <corecrt_math_defines.h>

#include "../WaterMeterCpp/MathUtils.h"
#include "../WaterMeterCpp/IntCoordinate.h"

void assertDoubleEqual(const double& a, const double& b, const std::string& label, const double& epsilon) {
	ASSERT_TRUE(isAboutEqual(a, b, epsilon)) << label << ": " << a << "!=" << b;
}

void assertCoordinatesEqual(const Coordinate& a, const Coordinate& b, const std::string& label, const double& epsilon) {
	assertDoubleEqual(a.x, b.x, label + std::string("(X)"), epsilon);
	assertDoubleEqual(a.y, b.y, label + std::string("(Y)"), epsilon);
}

void assertIntCoordinatesEqual(const IntCoordinate& a, const IntCoordinate& b, const std::string& label) {
	ASSERT_EQ(a.x, b.x) << label + std::string("(X)");
	ASSERT_EQ(a.y, b.y) << label + std::string("(Y)");
}

void assertAnglesEqual(const Angle& a, const Angle& b, const std::string& label, const double& epsilon) {
	// avoid the asymptotes for tan
	if (isAboutEqual(b.value, M_PI/2, epsilon) || isAboutEqual(b.value, -M_PI / 2, epsilon)) {
		assertDoubleEqual(a.value, b.value, label + "(asymptote)", epsilon);
	}
	else {
		// cater for differences of M_PI in the result which is OK for the tilt getAngle of an ellipse
		assertDoubleEqual(tan(a.value), tan(b.value), label + "(tan)", epsilon);
	}
}
