#include "EllipseFit.h"

#include <iostream>

// see https://autotrace.sourceforge.net/WSCG98.pdf for an explanation of the algorithm
// optimized version of https://github.com/mericdurukan/ellipse-fitting

EllipseFit::EllipseFit() : _c1Inverse(3, 3), _design1(BUFFER_SIZE, 3), _design2(BUFFER_SIZE, 3) {
	// the inverse constraint matrix C is constant so we calculate it once.
	Eigen::MatrixXd c1(3, 3);
	c1 << 0, 0, 2, 0, -1, 0, 2, 0, 0;
	_c1Inverse = c1.inverse();
	_design2.col(2) = Eigen::VectorXd::Ones(BUFFER_SIZE);
}

bool EllipseFit::addMeasurement(const Coordinate& p) {
	if (_size >= BUFFER_SIZE) return false;
	// Design matrix (D in the article). Building up incrementally to minimize compute at fit
	_design1( _size, 0) = p.x * p.x;
	_design1(_size, 1) = p.x * p.y;
	_design1(_size, 2) = p.y * p.y;
	_design2( _size, 0) = p.x;
	_design2( _size, 1) = p.y;
	_size++;
	return true;
}

void EllipseFit::begin() {
	_size = 0;
}

QuadraticEllipse EllipseFit::fit() {

    // Scatter matrix (S in the article)
 	const Eigen::MatrixXd scatter1 = _design1.transpose() * _design1;
	const Eigen::MatrixXd scatter2 = _design1.transpose() * _design2;
	const Eigen::MatrixXd scatter3 = _design2.transpose() * _design2;

	// reduced scatter matrix (M in the article)
	const Eigen::MatrixXd reducedScatter = _c1Inverse * (scatter1 - scatter2 * scatter3.inverse() * scatter2.transpose());

	const Eigen::EigenSolver<Eigen::MatrixXd> solver(reducedScatter);
	Eigen::MatrixXd eigenvector = solver.eigenvectors().real();

	// 4ac - b^2
	Eigen::VectorXd condition = 4 * (eigenvector.row(0).array() * eigenvector.row(2).array()) - eigenvector.row(1).array().pow(2);

	// a1 and a2 are the coefficients: a1 = (a,b,c), a2 = (d,e,f).
	Eigen::VectorXd a1;

	// there should be one where the condition is positive, that's the one we need

	for (int i = 0; i < 3; i++) {
		if (condition(i) > 0) {
			a1 = eigenvector.col(i);
			break;
		}
		// we didn't found it (should not happen)
		if (i == 2) {
			printf("No positive eigenvector found");
			return {0, 0, 0, 0, 0, 0};
		}
	}

    const Eigen::VectorXd a2 = -1 * scatter3.inverse() * scatter2.transpose() * a1;

	return {a1(0), a1(1), a1(2), a2(0), a2(1), a2(2)};
}
