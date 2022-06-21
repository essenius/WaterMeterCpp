// Copyright 2021-2022 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#ifndef HEADER_FLOWMETER
#define HEADER_FLOWMETER
#include "ChangePublisher.h"
#include "EventClient.h"

constexpr float PI = 3.1415926536f;

// small value to make sure that atan2 returns a valid value
constexpr float HIGH_PASS_START_VALUE = 0.0001f;

class FlowMeter : public EventClient {
public:
	explicit FlowMeter(EventServer* eventServer);
	float score(float input, float a, float b) const;
	float correctedDifference(float previousAngle, float currentAngle);
	void begin(int noiseRange, float gain);
	void addSample(int measurement);
	void addSample(Coordinate sample);

	float getZeroThreshold() const { return _zeroThreshold; }
	bool hasFlow() const { return _flow; }
	bool isExcluded() const { return _exclude; }
	bool isOutlier() const { return _outlier; }
	bool isPeak() const { return _peak; }
	bool isSearching() const { return _findNext; }
	int getZone() const { return _zone;  }

	// TODO: delete
	bool hasEnteredBand() const { return _hasEnteredBand; }

	void update(Topic topic, long payload) override;
	void update(Topic topic, Coordinate payload) override;
	bool wasReset() const { return _firstCall; }

	static constexpr int SAMPLE_PERIOD_MICROS = 10000;
	static constexpr float SAMPLE_PERIOD_SECONDS = SAMPLE_PERIOD_MICROS / 1000000.0f;

	FloatCoordinate getSmoothSample() const { return _smooth; }
	FloatCoordinate getHighPassSample() const { return _highpass; }
	float getAngle() const { return _angle; }
	float getDistance() const { return _distance; }
	float getSmoothDistance() const { return _smoothRelativeDistance; }
	

protected:
	unsigned int _consecutiveOutliers = 0;
	ChangePublisher<bool> _exclude;
	ChangePublisher<bool> _flow;
	ChangePublisher<bool> _peak;
	float _fastSmooth = 0.0f;
	float _previousFastSmooth = 0.0f;
	float _fastDerivative = 0.0f;
	bool _firstCall = true;
	bool _outlier = false;
	float _smoothAbsFastDerivative = 0.0f;
	float _smoothFastDerivative = 0.0f;

	float _zeroThreshold = 0.0f;
	float _flowThreshold = _zeroThreshold;
	bool _hasEnteredBand = false;
	bool _fastFlow = false;
	float _slowSmooth = 0.0f;
	float _combinedDerivative = 0.0f;
	float _previousSlowSmooth = 0.0f;
	float _smoothAbsCombinedDerivative = 0.0f;
	float _outlierThreshold = 0.0f;
	float _previousCombinedDerivative = 0.0f;
	float _amplitude = 0.0;

	FloatCoordinate _smooth = {0.0f, 0.0f};
	FloatCoordinate _previousSmooth = {0.0f, 0.0f};
	FloatCoordinate _highpass = { HIGH_PASS_START_VALUE, HIGH_PASS_START_VALUE};
	float _smoothRelativeDistance = 0.0f;
	float _distance = 0.0f;
	int _zone = 0;
	int _state = _zone;
	int _findNext = true;
	bool _noise = false;
	bool _stalled = false;
	float _angle = -PI;
	float _cordifLowPass;
	float _averageAbsoluteDistance = 0.0f;

	void detectOutlier(int measurement);
	void detectOutlier(FloatCoordinate measurement);
	void detectPeaks(FloatCoordinate sample);
	void detectPeaks(int measurement);

	static float highPassFilter(float measure, float previous, float filterValue, float alpha);
	static float lowPassFilter(float measure, float filterValue, float alpha);
	void markAnomalies();
	void resetAnomalies();
	void resetFilters(int initialMeasurement);
	void resetFilters(FloatCoordinate initialSample);
};

#endif
