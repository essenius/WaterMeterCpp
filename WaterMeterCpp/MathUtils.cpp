#include <cmath>

bool aboutEqual(const double& a, const double& b, const double& epsilon) {
	return fabs(a - b) <= epsilon;
}

double sqr(const double& a) {
	return a * a;
}

int modulo(const int a, const int b) {
	return (b + a % b) % b;
}