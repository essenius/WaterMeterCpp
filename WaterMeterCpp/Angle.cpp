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

#include <cstdio>
#include <cmath>
#include "Angle.h"
#include "MathUtils.h"

Angle Angle::operator- (const Angle& other) const {
	return { operator-(other.value) };
}

double Angle::operator- (const double& other) const {
	auto returnValue = value - other;
	if (returnValue < -M_PI) returnValue += 2 * M_PI;
	if (returnValue > M_PI) returnValue -= 2 * M_PI;
	return  returnValue;
}

unsigned int Angle::getQuadrant() const {
	if (value < -M_PI / 2.0) return 3;
	if (value < 0.0) return 4;
	if (value < M_PI / 2.0) return 1;
	return 2;
}

double Angle::sin() const {
	return std::sin(value);
}

double Angle::cos() const {
	return std::cos(value);
}

int16_t Angle::degreesTimes10() const {
	return static_cast <int16_t>(value * 1800.0 / M_PI);
}
