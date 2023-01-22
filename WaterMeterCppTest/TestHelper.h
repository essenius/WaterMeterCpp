#ifndef TEST_HELPER_H
#define TEST_HELPER_H

#include "../WaterMeterCpp/IntCoordinate.h"
#include "../WaterMeterCpp/MathUtils.h"

void assertDoubleEqual(const double& a, const double& b, const std::string& label, const double& epsilon = EPSILON);
void assertCoordinatesEqual(const Coordinate& a, const Coordinate& b, const std::string& label, const double& epsilon = EPSILON);
void assertIntCoordinatesEqual(const IntCoordinate& a, const IntCoordinate& b, const std::string& label);
void assertAnglesEqual(const Angle& a, const Angle& b, const std::string& label, const double& epsilon = EPSILON);

#endif