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
