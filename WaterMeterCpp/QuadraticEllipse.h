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

// The fitting process delivers coefficients. This class translates those to coordinates (getCenter, getRadius, and getAngle of main getRadius).

#ifndef QUADRATIC_ELLIPSE_H
#define QUADRATIC_ELLIPSE_H

#include "Coordinate.h"

struct QuadraticEllipse {
	double a{};
	double b{};
	double c{};
	double d{};
	double f{};
	double g{};

	QuadraticEllipse(const double& a1, const double& b1, const double& c1, const double& d1, const double& f1, const double& g1);
	QuadraticEllipse() = default;
	Angle getAngle();
	Coordinate getCenter() const;
	Coordinate getRadius();
private:
	double getDiscriminant() const;
	Coordinate _radius{};
	bool _radiusCalculated = false;
	bool _switchedAxes = false;
};
#endif