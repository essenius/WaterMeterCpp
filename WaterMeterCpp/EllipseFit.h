#ifndef ELLIPSEFIT_H
#define ELLIPSEFIT_H

#ifdef ESP32
#include <ArduinoEigenDense.h>
#else
// ReSharper disable once CppUnusedIncludeDirective - false positive
#include <Eigen/Dense>
#include <Eigen/Core>
#endif

#include "CartesianEllipse.h"
#include "QuadraticEllipse.h"

class EllipseFit {

public:
	explicit EllipseFit();
	bool addMeasurement(const Coordinate& p);
	void begin();
	QuadraticEllipse fit();
	bool bufferIsFull() const { return _size >= BUFFER_SIZE; }
	unsigned int pointCount() const { return _size; }
    static unsigned int size() { return BUFFER_SIZE; }

private:
	static constexpr unsigned int BUFFER_SIZE = 32;
	Eigen::MatrixXd _c1Inverse;
	Eigen::MatrixXd _design1;
	Eigen::MatrixXd _design2;

	unsigned int _size = 0;
};
#endif
