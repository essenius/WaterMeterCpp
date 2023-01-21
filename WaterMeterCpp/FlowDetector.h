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

#ifndef FLOW_DETECTOR_H
#define FLOW_DETECTOR_H

#include "CartesianEllipse.h"
#include "EllipseFit.h"
#include "EventServer.h"
#include "IntCoordinate.h"

class FlowDetector : public EventClient {
public:
	FlowDetector(EventServer* eventServer, EllipseFit* ellipseFit);
	void begin(unsigned int noiseRange);
	bool foundAnomaly() const { return _foundAnomaly; }
	bool foundPulse() const { return _foundPulse; }
	bool isSearching() const { return _searchingForPulse; }
	Coordinate movingAverage() const { return _movingAverage; }
	void update(Topic topic, long payload) override;
	void update(Topic topic, IntCoordinate payload) override;
	bool wasReset() const { return _firstCall; }
	bool wasSkipped() const { return _wasSkipped; }
protected:
	void addSample(const IntCoordinate& sample);
	Coordinate calcMovingAverage();
	void detectPulse(Coordinate point);
	CartesianEllipse executeFit() const;
	void findPulseByCenter(const Coordinate& point);
	void findPulseByPrevious(const Coordinate& point);
	bool isRelevant(const Coordinate& point);
	void updateEllipseFit(Coordinate point);
    void updateMovingAverageArray(const IntCoordinate& sample);

    static constexpr unsigned int MOVING_AVERAGE_SIZE = 4;
	IntCoordinate _movingAverageArray[MOVING_AVERAGE_SIZE] = {};
	int8_t _movingAverageIndex = 0;
	bool _justStarted = true;
	CartesianEllipse _confirmedGoodFit;
	EllipseFit* _ellipseFit;
	unsigned int _previousQuadrant = 0;
	unsigned int _previousQuadrantFromStart = 1;
	Coordinate _startPoint;
	Coordinate _referencePoint;

	unsigned int _currentIndex = 0; 
	Coordinate _previousPoint;
	Angle _startTangent = {NAN};
	unsigned int _waitCount = 0;
	bool _searchingForPulse = true;
	Angle _previousAngleWithCenter = { NAN };
	double _angleDistanceTravelled = 0;
	unsigned int _pointCount = 0;
	bool _foundAnomaly = false;
	double _distanceThreshold = 2.378414; // noise factor = 4, distance = sqrt(32), MA reduces noise to sqrt(distance)
    bool _firstCall = true;
	bool _firstRound = true;
	Coordinate _movingAverage = { NAN, NAN };
	bool _foundPulse = false;
    bool _wasSkipped = false;
	double _tangentDistanceTravelled = 0;
    Angle _previousAngleWithPreviousFromStart = {};

};



#endif
