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

// When the magnetosensor detects a clockwise elliptical move in the X-Y plane, water is flowing.
// The parameters of the ellipse are estimated via a fitting mechanism using a series of samples (see EllipseFit).
// We generate an event every time the cycle moves from the 4th to the 3rd getQuadrant.
// The detector also tries to filter out anomalies by ignoring points that are too far away from the latest fitted ellipse.

// The signal is disturbed by the presence of electrical devices nearby. Therefore we sample at 100 Hz (twice the rate of the mains frequency),
// and use a moving average over 4 samples to clean up the signal.

#ifndef FLOW_DETECTOR_H
#define FLOW_DETECTOR_H

#include "CartesianEllipse.h"
#include "EllipseFit.h"
#include "EventServer.h"

class FlowDetector : public EventClient {
public:
	FlowDetector(EventServer* eventServer, EllipseFit* ellipseFit);
	void begin(unsigned int noiseRange);
	bool foundAnomaly() const { return _foundAnomaly; }
	bool foundPulse() const { return _foundPulse; }
	bool isSearching() const { return _searchingForPulse; }
	Coordinate getMovingAverage() const { return _movingAverage; }
	void update(Topic topic, long payload) override;
	void update(Topic topic, IntCoordinate payload) override;
	bool wasReset() const { return _wasReset; }
	bool wasSkipped() const { return _wasSkipped; }
	IntCoordinate ellipseCenterTimes10() const { return IntCoordinate::times10(_confirmedGoodFit.center); }
	IntCoordinate ellipseRadiusTimes10() const { return IntCoordinate::times10(_confirmedGoodFit.radius); }
	int16_t ellipseAngleTimes10() const { return _confirmedGoodFit.angle.degreesTimes10(); }
protected:
	void addSample(const IntCoordinate& sample);
    Coordinate calcMovingAverage();
	void detectPulse(Coordinate point);
	CartesianEllipse executeFit() const;
	void findPulseByCenter(const Coordinate& point);
	void findPulseByPrevious(const Coordinate& point);
	bool isRelevant(const Coordinate& point);
	void processMovingAverageSample(Coordinate averageSample);
	void reportAnomaly();
    int16_t noFitParameter(double angleDistance, bool fitSucceeded) const;
    void updateEllipseFit(Coordinate point);
    void updateMovingAverageArray(const IntCoordinate& sample);

    static constexpr unsigned int MovingAverageSize = 4;
	static constexpr double MovingAverageNoiseReduction = 2; // = sqrt(MovingAverageSize)
	IntCoordinate _movingAverageArray[MovingAverageSize] = {};
	int8_t _movingAverageIndex = 0;
	bool _justStarted = true;
	CartesianEllipse _confirmedGoodFit;
	EllipseFit* _ellipseFit;
	unsigned int _previousQuadrant = 0;
	Coordinate _startPoint = {};
	Coordinate _referencePoint = {};

	Coordinate _previousPoint = {};
	Angle _startTangent = {NAN};
	unsigned int _waitCount = 0;
	bool _searchingForPulse = true;
	Angle _previousAngleWithCenter = { NAN };
	double _angleDistanceTravelled = 0;
	bool _foundAnomaly = false;
	double _distanceThreshold = 2.12132; // noise range = 3, getDistance = sqrt(18), MA(4) reduces noise with factor 2
    bool _firstCall = true;
	bool _firstRound = true;
	Coordinate _movingAverage = { NAN, NAN };
	bool _foundPulse = false;
    bool _wasSkipped = false;
	double _tangentDistanceTravelled = 0;
    Angle _previousAngleWithPreviousFromStart = {};
	bool _wasReset = true;
};



#endif
