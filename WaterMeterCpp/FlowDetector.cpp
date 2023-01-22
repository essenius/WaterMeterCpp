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

// It was empirically established that relative measurements are less reliable (losing slow moves in the noise),
// so now using absolutes measurements. Since absolute values are not entirely predictable, the tactic is to
// follow the clockwise elliptic move that the signal makes. We do this by regularly feeding relevant data points
// to an ellipse fitting algorithm. We then use the angle between the center of the ellipse and the data point
// to detect a cycle: if it is at the lowest Y point (move from quadrant 4 to 3) we count a pulse.
//
// We only include points that are at least the noise distance away from the previously included point.
// By doing that, the impact of noise (and the risk of outliers in angles) is greatly reduced. Further, the algorithm
// can deal much better with slow flow.
//
// To deal with remaining jitter, we start searching at the top of the ellipse, and we stop searching if a pulse was given.
//
// We also use the fitted ellipse to determine outliers: if a point is too far away from the ellipse it is an outlier.
//
// Before we have a good fit, we use the (less accurate) mechanism of taking the angle with the previous data point.
// It should be close to the angle to the center - PI / 2, and since we subtract, the difference of PI / 2 doesn't matter.
// When that moves from quadrant 3 to 2, we have a pulse. we accept the risk that in the first cycle we get an outlier. 

#include <ESP.h>
#include "FlowDetector.h"
#include "MathUtils.h"

constexpr double MIN_CYCLE_FOR_FIT = 0.6;

FlowDetector::FlowDetector(EventServer* eventServer, EllipseFit* ellipseFit): EventClient(eventServer), _ellipseFit(ellipseFit) {
	_eventServer = eventServer;
}

// Public methods

void FlowDetector::begin(const unsigned int noiseRange = 3) {
	// we assume that the noise range for X and Y is the same.
	// If the distance between two points is beyond this, it is beyond noise
	_distanceThreshold = sqrt(2.0 * noiseRange * noiseRange) / MOVING_AVERAGE_NOISE_REDUCTION;
	_eventServer->subscribe(this, Topic::Sample);
	_eventServer->subscribe(this, Topic::SensorWasReset);
}

void FlowDetector::update(const Topic topic, const long payload) {
	if (topic == Topic::SensorWasReset) {
		// If we needed to reset the sensor, also reset the measurement process when the next sample comes in
		_firstCall = true;
		_justStarted = true;
		_currentIndex = 0;
	}
}

void FlowDetector::update(const Topic topic, const IntCoordinate payload) {
	if (topic == Topic::Sample) {
		addSample(payload);
	}
}

// Private methods

void FlowDetector::addSample(const IntCoordinate& sample) {
	_currentIndex++;
	if (sample.isSaturated()) {
		_wasSkipped = true;
		_foundAnomaly = true;
		return;
	}
	if (_firstCall) {
		_movingAverageIndex = 0;
		_firstRound = true;
		_firstCall = false;
	}
	updateMovingAverageArray(sample);
	// if index is 0, we made the first round and the buffer is full. Otherwise we wait.
	if (_firstRound && _movingAverageIndex != 0) {
		_wasSkipped = true;
		return;
	}

    const auto averageSample = calcMovingAverage();

    if (_firstRound) {
		// We have the first valid moving average. Start the process.
		_ellipseFit->begin();
		_startPoint = averageSample;
		_referencePoint = _startPoint;
		_previousPoint = _startPoint;
		_firstRound = false;
		_wasSkipped = true;
		return;
	}

	if (!isRelevant(averageSample)) return;
	detectPulse(averageSample);

	if (!_ellipseFit->addMeasurement(averageSample)) {
		// this should not happen - bufferIsFull would trigger on time
		/* TODO: remove print statement */
		printf("Too many measurements!\n");
	}

	if (_ellipseFit->bufferIsFull()) {
		updateEllipseFit(averageSample);
	}

	_previousPoint = averageSample;
	_wasSkipped = false;
}

Coordinate FlowDetector::calcMovingAverage() {
	_movingAverage = { 0,0 };
	for (const auto i : _movingAverageArray) {
		_movingAverage.x += static_cast<double>(i.x);
		_movingAverage.y += static_cast<double>(i.y);
	}
	_movingAverage.x /= MOVING_AVERAGE_SIZE;
	_movingAverage.y /= MOVING_AVERAGE_SIZE;
	return _movingAverage;
}


void FlowDetector::detectPulse(const Coordinate point) {
	if (_confirmedGoodFit.hasData) {
		findPulseByCenter(point);
	}
	else {
	    findPulseByPrevious(point);
	}
}

CartesianEllipse FlowDetector::executeFit() const {
	const auto fittedEllipse = _ellipseFit->fit();

	const CartesianEllipse returnValue(fittedEllipse);
	printf("\n[%4d] Fit - ", _currentIndex);
	returnValue.print();
 	printf(", circumference: %.2f, angle distance travelled: %.3f", returnValue.circumference(), _angleDistanceTravelled);
	_ellipseFit->begin();
	return returnValue;
}

void FlowDetector::findPulseByCenter(const Coordinate& point) {
	const auto angleWithCenter = point.angleFrom(_confirmedGoodFit.center);
	const int quadrant = angleWithCenter.quadrant();
	// previous angle is initialized in the first fit, so always has a valid value when coming here
	const auto angleDistance = angleWithCenter - _previousAngleWithCenter.value;
	_angleDistanceTravelled += angleDistance;
	if (!_searchingForPulse) {
		_foundPulse = false;
		// start searching at the top of the ellipse. This takes care of jitter
		if (quadrant == 1 && _previousQuadrant == 2) _searchingForPulse = true;
	}
	else {
		// reference point is the bottom of the ellipse
		_foundPulse = quadrant == 3 && _previousQuadrant == 4;
		if (_foundPulse) {
			_eventServer->publish(Topic::Pulse, true);
 			_searchingForPulse = false;
			printf("\n[%4d] Found Pulse. points: %d ", _currentIndex, _pointCount);
			_pointCount = 0;
		}
	}
	_previousQuadrant = quadrant;
	_previousAngleWithCenter = angleWithCenter;
}

void FlowDetector::findPulseByPrevious(const Coordinate& point) {

	const auto angleWithPreviousFromStart = point.angleFrom(_previousPoint) - _startTangent;
	_tangentDistanceTravelled += (angleWithPreviousFromStart - _previousAngleWithPreviousFromStart).value;
	printf(" angle: %.3f, traveled: %.3f, fitPoints=%d\n", angleWithPreviousFromStart.value, _tangentDistanceTravelled, _ellipseFit->pointCount());
	_previousAngleWithPreviousFromStart = angleWithPreviousFromStart;

	const auto quadrant = point.angleFrom(_previousPoint).quadrant();

	// this can be jittery, so use a flag to check whether we counted, and reset the counter at the other side of the ellipse

    _foundPulse = _searchingForPulse && quadrant == 2 && _previousQuadrant == 3;
	if (_foundPulse) {
		printf("\n[%4d] Found Pulse (previous) fitpoints=%d.\n", _currentIndex, _ellipseFit->pointCount());
		_eventServer->publish(Topic::Pulse, false);
		_searchingForPulse = false;
		_pointCount = 0;
	} else if (!_searchingForPulse && (quadrant == 1 || quadrant == 4)) {
		_searchingForPulse = true;
	}
	_previousQuadrant = quadrant;
}

bool FlowDetector::isRelevant(const Coordinate& point) {

	const auto distance = point.distanceFrom(_referencePoint);
	// if we are too close to the previous point, discard
	if (distance < _distanceThreshold) {
		printf(".");
		_wasSkipped = true;
		return false;
	}
	// if we have a confirmed fit and the point is too far away from it, we have an anomaly. Discard.
	if (_confirmedGoodFit.hasData) {
		const auto distanceFromEllipse = _confirmedGoodFit.distanceFrom(point);
		_foundAnomaly = _confirmedGoodFit.hasData && distanceFromEllipse > _distanceThreshold;
		if (_foundAnomaly) {
			_eventServer->publish(Topic::Exclude, true);
			_wasSkipped = true;
			printf("[%d!%.3f]", _currentIndex, distanceFromEllipse);
			return false;
		}
	}

	// if we have just started, we might have impact from the AC current due to the moving average. Wait until stable.
	if (_justStarted) {
		_waitCount++;
		if (_waitCount <= MOVING_AVERAGE_SIZE) {
			printf("#");
			_wasSkipped = true;
			return false;
		}
		_startTangent = point.angleFrom(_referencePoint);
		printf("\n[%4d] Start move (tangent=%.3f, fitPoints=%d)\n", _currentIndex, _startTangent.value, _ellipseFit->pointCount());
		_justStarted = false;
		_waitCount = 0;
	}
	_referencePoint = point;
	_pointCount++;
	return true;
}

void FlowDetector::updateEllipseFit(const Coordinate point) {
	if (!_confirmedGoodFit.hasData) {
		// The first time we always run a fit. Re-run if the first time(s) didn't result in a good fit
		const auto fittedEllipse = executeFit();
		const auto center = fittedEllipse.center;
		// number of points per ellipse defines whether the fit is reliable.
		const auto passedCycles = _tangentDistanceTravelled / (2 * M_PI);
		printf(", passed cycles: %.3f ", passedCycles);
		if (abs(passedCycles) >= MIN_CYCLE_FOR_FIT) {
			_confirmedGoodFit = fittedEllipse;
			printf("\n[%4d] Good first Fit - %.1f", _currentIndex, _angleDistanceTravelled * 180);

			_previousAngleWithCenter = point.angleFrom(center);
			_previousQuadrant = _previousAngleWithCenter.quadrant();
		} else {
			// we need another round
			/* TODO: remove print*/
			printf("\n[%4d] Bad first Fit - %.1f", _currentIndex, _angleDistanceTravelled * 180);
			_eventServer->publish(Topic::NoFit, static_cast <int16_t>(round(_tangentDistanceTravelled * 180)));
		}
		_tangentDistanceTravelled = 0;
	}
	else {
		// If we already had a reliable fit, check whether the new data is good enough to warrant a new fit.
		// Otherwise we keep the old one. 'Good enough' means we covered at least 60% of a cycle.
		// we do this because the ellipse centers are moving a bit and we want to minimize deviations.
		if (fabs(_angleDistanceTravelled / (2 * M_PI)) > MIN_CYCLE_FOR_FIT) {
			_confirmedGoodFit = executeFit();
			printf("\n[%4d] Good Fit - %.1f", _currentIndex, _angleDistanceTravelled * 180);
		}
		else {
			printf("\n[%4d] Bad Fit - %.1f", _currentIndex, _angleDistanceTravelled * 180);

			_eventServer->publish(Topic::NoFit, static_cast <int16_t>(round(_angleDistanceTravelled * 180)));
			_ellipseFit->begin();
		}
		_angleDistanceTravelled = 0;
	}
}

void FlowDetector::updateMovingAverageArray(const IntCoordinate& sample) {
	_movingAverageArray[_movingAverageIndex] = sample;
	++_movingAverageIndex %= 4;
}
