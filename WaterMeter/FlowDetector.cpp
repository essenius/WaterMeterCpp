// Copyright 2021-2024 Rik Essenius
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
// When that moves from quadrant 3 to 2, we have a pulse. we accept the (small) risk that in the first cycle we get an outlier. 

#include <ESP.h>
#include "FlowDetector.h"
#include <EllipseFit.h>
#include <MathUtils.h>

namespace WaterMeter {
	using EllipseMath::EllipseFit;
	using EllipseMath::CartesianEllipse;

	constexpr double MinCycleForFit = 0.6;

	FlowDetector::FlowDetector(EventServer* eventServer, EllipseFit* ellipseFit) : EventClient(eventServer), _ellipseFit(ellipseFit) {
		_eventServer = eventServer;
	}

	// Public methods

	void FlowDetector::begin(const unsigned int noiseRange = 3) {
		// we assume that the noise range for X and Y is the same.
		// If the distance between two points is beyond this, it is beyond noise
		_distanceThreshold = sqrt(2.0 * noiseRange * noiseRange) / MovingAverageNoiseReduction;
		_eventServer->subscribe(this, Topic::Sample);
		_eventServer->subscribe(this, Topic::SensorWasReset);
	}

    void FlowDetector::resetMeasurement() {
        _firstCall = true;
        _wasReset = true;
        _justStarted = true;
        _consecutiveOutlierCount = 0;
		_confirmedGoodFit = CartesianEllipse();
    }

	void FlowDetector::update(const Topic topic, const long payload) {
		if (topic == Topic::SensorWasReset) {
			// If we needed to reset the sensor, also reset the measurement process when the next sample comes in
			resetMeasurement();
		}
	}

	void FlowDetector::update(const Topic topic, const SensorSample payload) {
		if (topic == Topic::Sample) {
			addSample(payload);
		}
	}

	// Private methods

	void FlowDetector::addSample(const SensorSample& sample) {
		const auto state = sample.state();
		if (state!= SensorState::Ok) {
			reportAnomaly(state);
			return;
		}
		_foundAnomaly = false;
		_wasReset = _firstCall;
		if (_firstCall) {
			// skip samples as long as we get a flatline. Happens sometimes just after startup
			if (sample.x == 0 && sample.y == 0) {
				reportAnomaly(SensorState::FlatLine);
				return;
			}
			_movingAverageIndex = 0;
			_firstRound = true;
			_firstCall = false;
		}
		updateMovingAverageArray(sample);
		// if index is 0, we made the first round and the buffer is full. Otherwise, we wait.
		if (_firstRound && _movingAverageIndex != 0) {
			_wasSkipped = true;
			return;
		}

		const auto averageSample = calcMovingAverage();
		processMovingAverageSample(averageSample);
	}

	Coordinate FlowDetector::calcMovingAverage() {
		_movingAverage = { 0,0 };
		for (const auto i : _movingAverageArray) {
			_movingAverage.x += static_cast<double>(i.x);
			_movingAverage.y += static_cast<double>(i.y);
		}
		_movingAverage.x /= MovingAverageSize;
		_movingAverage.y /= MovingAverageSize;
		return _movingAverage;
	}

	void FlowDetector::detectPulse(const Coordinate point) {
		if (_confirmedGoodFit.isValid()) {
			findPulseByCenter(point);
		}
		else {
			findPulseByPrevious(point);
		}
	}

	CartesianEllipse FlowDetector::executeFit() const {
		const auto fittedEllipse = _ellipseFit->fit();
		const CartesianEllipse returnValue(fittedEllipse);
		_ellipseFit->begin();
		return returnValue;
	}

    void FlowDetector::waitToSearch(const unsigned int quadrant, const unsigned int quadrantDifference) {
		// Consider the risk that a quadrant gets skipped because of an anomaly
        // start searching at the top of the ellipse. This takes care of jitter
        const auto passedTop =  
			(quadrantDifference == 1 && quadrant == 1) ||
			(quadrantDifference == 2 && (quadrant == 1 || quadrant == 4));
		if (passedTop) {
            _searchingForPulse = true;
        }
	}

	bool passedBottom(const unsigned int quadrant, const unsigned int quadrantDifference) {
		// Consider the risk that a quadrant gets skipped
		return (quadrantDifference == 1 && quadrant == 3) ||
			(quadrantDifference == 2 && (quadrant == 3 || quadrant == 2));
	}

	void FlowDetector::findPulseByCenter(const Coordinate& point) {
		const auto angleWithCenter = point.getAngleFrom(_confirmedGoodFit.getCenter());
		const auto quadrant = angleWithCenter.getQuadrant();
		const auto quadrantDifference = (_previousQuadrant - quadrant) % 4;
		// previous angle is initialized in the first fit, so always has a valid value when coming here
		const auto angleDistance = angleWithCenter - _previousAngleWithCenter.value;
		_angleDistanceTravelled += angleDistance;
		if (!_searchingForPulse) {
			_foundPulse = false;
			waitToSearch(quadrant, quadrantDifference);
		}
		else {
			// reference point is the bottom of the ellipse
			_foundPulse = passedBottom(quadrant, quadrantDifference);
			if (_foundPulse) {
				_eventServer->publish(Topic::Pulse, true);
				_searchingForPulse = false;
			}
		}
		_previousQuadrant = quadrant;
		_previousAngleWithCenter = angleWithCenter;
	}

	void FlowDetector::findPulseByPrevious(const Coordinate& point) {

		const auto angleWithPreviousFromStart = point.getAngleFrom(_previousPoint) - _startTangent;
		_tangentDistanceTravelled += (angleWithPreviousFromStart - _previousAngleWithPreviousFromStart).value;
		_previousAngleWithPreviousFromStart = angleWithPreviousFromStart;

		const auto quadrant = point.getAngleFrom(_previousPoint).getQuadrant();

		// this can be jittery, so use a flag to check whether we counted, and reset the counter at the other side of the ellipse

		_foundPulse = _searchingForPulse && quadrant == 2 && _previousQuadrant == 3;
		if (_foundPulse) {
			_eventServer->publish(Topic::Pulse, false);
			_searchingForPulse = false;
		}
		else if (!_searchingForPulse && (quadrant == 1 || quadrant == 4)) {
			_searchingForPulse = true;
		}
		_previousQuadrant = quadrant;
	}

	// We have an outlier if the point is too far away from the confirmed fit.
    bool FlowDetector::isOutlier(const Coordinate point) {
		const auto distanceFromEllipse = _confirmedGoodFit.getDistanceFrom(point);
		if (distanceFromEllipse <= _distanceThreshold * 2) return false;

	    const auto reportedDistance = static_cast<uint16_t>(std::min(lround(distanceFromEllipse * 100), 4095l));
		reportAnomaly(SensorState::Outlier, reportedDistance);
		_consecutiveOutlierCount++;
		return true;
    }

	// if we have just started, we might have impact from the AC current due to the moving average. Wait until stable.
	// Calculates the start tangent once waited long enough.
    bool FlowDetector::isStartingUp(const Coordinate point) {
		if (_justStarted) {
			_waitCount++;
			if (_waitCount <= MovingAverageSize) {
				_wasSkipped = true;
				return true;
			}
			_startTangent = point.getAngleFrom(_referencePoint);
			_justStarted = false;
			_waitCount = 0;
		}
		return false;
	}

    bool FlowDetector::isRelevant(const Coordinate& point) {

		const auto distance = point.getDistanceFrom(_referencePoint);
		// if we are too close to the previous point, discard
		if (distance < _distanceThreshold) {
			_wasSkipped = true;
			return false;
		}
		if (_confirmedGoodFit.isValid() && isOutlier(point)) {
    		return false;
		}

		if (isStartingUp(point)) {
			return false;
		}
		_referencePoint = point;
		return true;
	}

	void FlowDetector::processMovingAverageSample(const Coordinate averageSample) {
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

		if (!isRelevant(averageSample)) {
			// not leaving potential loose ends
			_foundPulse = false;
			// if we have too many outliers in a row, we might have drifted (e.g. the sensor was moved), so we reset the measurement
			if (_consecutiveOutlierCount > 0 && _consecutiveOutlierCount % MaxConsecutiveOutliers == 0) {
			    _eventServer->publish(Topic::Drifted, _consecutiveOutlierCount);
				resetMeasurement();
			}
			return;
		}
		_consecutiveOutlierCount = 0;
		detectPulse(averageSample);

		_ellipseFit->addMeasurement(averageSample);
		if (_ellipseFit->bufferIsFull()) {
			updateEllipseFit(averageSample);
		}
		_previousPoint = averageSample;
		_wasSkipped = false;
	}

	void FlowDetector::reportAnomaly(SensorState state, const uint16_t value) {
		_foundAnomaly = true;
		_wasSkipped = true;
		_eventServer->publish(Topic::Anomaly, static_cast<int16_t>(state) + (value << 4));
	}

	int16_t  FlowDetector::noFitParameter(const double angleDistance, const bool fitSucceeded) {
		return static_cast<int16_t>(round(abs(angleDistance * 180) * (fitSucceeded ? 1.0 : -1.0)));
	}

    void FlowDetector::runFirstFit(const Coordinate point) {
        const auto fittedEllipse = executeFit();
        const auto center = fittedEllipse.getCenter();
        // number of points per ellipse defines whether the fit is reliable.
        const auto passedCycles = _tangentDistanceTravelled / (2 * M_PI);
        const auto fitSucceeded = fittedEllipse.isValid();
        if (fitSucceeded && abs(passedCycles) >= MinCycleForFit) {
            _confirmedGoodFit = fittedEllipse;
            _previousAngleWithCenter = point.getAngleFrom(center);
            _previousQuadrant = _previousAngleWithCenter.getQuadrant();
        }
        else {
            // we need another round
            _eventServer->publish(Topic::NoFit, noFitParameter(_tangentDistanceTravelled, fitSucceeded));
        }
        _tangentDistanceTravelled = 0;
    }

    void FlowDetector::runNextFit() {
        // If we already had a reliable fit, check whether the new data is good enough to warrant a new fit.
        // Otherwise, we keep the old one. 'Good enough' means we covered at least 60% of a cycle.
        // we do this because the ellipse centers are moving a bit, and we want to minimize deviations.
        if (fabs(_angleDistanceTravelled / (2 * M_PI)) > MinCycleForFit) {
            const auto fittedEllipse = executeFit();
            if (fittedEllipse.isValid()) {
                _confirmedGoodFit = fittedEllipse;
            }
            else {
                _eventServer->publish(Topic::NoFit, noFitParameter(_angleDistanceTravelled, false));
            }
        }
        else {
            // even though we didn't run a fit, we mark it as succeeded to see the difference with one that failed a fit
            _eventServer->publish(Topic::NoFit, noFitParameter(_angleDistanceTravelled, true));
            _ellipseFit->begin();
        }
        _angleDistanceTravelled = 0;
    }

	void FlowDetector::updateEllipseFit(const Coordinate point) {
		// The first time we always run a fit. Re-run if the first time(s) didn't result in a good fit
		if (!_confirmedGoodFit.isValid()) {
			runFirstFit(point);
		}
		else {
			runNextFit();
		}
	}

	void FlowDetector::updateMovingAverageArray(const SensorSample& sample) {
		_movingAverageArray[_movingAverageIndex] = sample;
		++_movingAverageIndex %= 4;
	}
}
