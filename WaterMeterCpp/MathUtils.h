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

#ifndef DOUBLE_UTILS_H
#define DOUBLE_UTILS_H

#ifndef ESP32
// ReSharper disable once CppUnusedIncludeDirective - on purpose, to get PI defined
#include <corecrt_math_defines.h>
#endif

constexpr double EPSILON = 1e-6;

bool aboutEqual(const double& a, const double& b, const double& epsilon = EPSILON);
double sqr(const double& a);
int modulo(int a, int b);
#endif
