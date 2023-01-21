// Copyright 2021-2023 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#ifndef COORDINATE_H
#define COORDINATE_H
#include "Angle.h"

struct Coordinate {
	double x = 0;
	double y = 0;

	//Coordinate();
	//Coordinate(const double& x, const double& y);
    bool operator==(const Coordinate& other) const;

	Angle angle() const;
	Angle angleFrom(const Coordinate& other) const;
	double distance() const;
	double distanceFrom(const Coordinate& other) const;
	Coordinate operator-() const;
	void print() const;
	Coordinate reciprocal() const;
	Coordinate rotate(double angle) const;
	Coordinate translate(const Coordinate& vector) const;
	Coordinate scale(const Coordinate& vector) const;
};
#endif
