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
#include "ExtremeSearcher.h"

// small value to make sure that atan2 returns a valid value
constexpr float HIGH_PASS_START_VALUE = 0.0001f;

// Arduino.h already has a macro PI which evaluates to a double
constexpr float PI_F = 3.1415926536f;


class FlowMeter : public EventClient {
public:
    explicit FlowMeter(EventServer* eventServer);
    float score(float input, float a, float b) const;
    float correctedDifference(float previousAngle, float currentAngle);
    void begin(int noiseRange, float gain);
    void addSample(Coordinate sample);

    bool hasFlow() const { return _flow; }
    bool isExcluded() const { return _exclude; }
    bool isOutlier() const { return _outlier; }
    bool isPeak() const { return _peak; }
    bool isSearching() const { return _findNext; }
    int getZone() const { return _zone; }

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
    static constexpr int8_t MOVING_AVERAGE_BUFFER_SIZE = 4;
    FloatCoordinate _movingAverage[MOVING_AVERAGE_BUFFER_SIZE] = {};
    int8_t _movingAverageIndex = 0;
    int8_t _movingAverageIndexStartupLeft = MOVING_AVERAGE_BUFFER_SIZE - 1;
    bool _flowStarted = false;
    FloatCoordinate _firstSample;
    int8_t _flowThresholdPassedCount = 0;

    FloatCoordinate _averageStartValue = {};

    ExtremeSearcher _maxYSearcher;
    ExtremeSearcher _maxXSearcher;
    ExtremeSearcher _minYSearcher;
    ExtremeSearcher _minXSearcher;

    FloatCoordinate _maxX;
    FloatCoordinate _maxY;
    FloatCoordinate _minX;
    FloatCoordinate _minY;
    bool _foundMaxX = false;
    bool _foundMaxY = false;
    bool _foundMinX = false;
    bool _foundMinY = false;
    bool _maxXCandidate = false;
    bool _maxYCandidate = false;
    bool _minXCandidate = false;
    bool _minYCandidate = false;


    unsigned int _consecutiveOutliers = 0;
    ChangePublisher<bool> _exclude;
    ChangePublisher<bool> _flow;
    ChangePublisher<bool> _peak;
    bool _firstCall = true;
    bool _outlier = false;
    float _outlierThreshold = 0.0f;
    // TODO: delete
    float _zeroThreshold = 0.0f;
    bool _firstRound = true;


    FloatCoordinate _smooth = {0.0f, 0.0f};
    FloatCoordinate _previousSmooth = {0.0f, 0.0f};
    FloatCoordinate _highpass = {HIGH_PASS_START_VALUE, HIGH_PASS_START_VALUE};
    float _smoothRelativeDistance = 0.0f;
    float _distance = 0.0f;
    int _zone = 0;
    int _state = _zone;
    int _findNext = true;
    bool _noise = false;
    bool _stalled = false;
    float _angle = -PI_F;
    float _cordifLowPass;
    float _averageAbsoluteDistance = 0.0f;
    FloatCoordinate _filteredSample = {};
    int _averageCount = 0;
    SearchTarget _searchTarget = None;
    ExtremeSearcher* _currentSearcher = nullptr;

    void detectOutlier(FloatCoordinate measurement);

    void detectPulse(FloatCoordinate sample);

    static int modulo(int number, int divisor);
    static bool isPulseCandidate(int stateDifference, int zone);
    void setFindNext(bool peakCandidate, int stateDifference, int zone);
    static SearchTarget getTarget(FloatCoordinate direction);
    FloatCoordinate movingAverage();
    FloatCoordinate lowPassFilter(FloatCoordinate measure, FloatCoordinate filterValue, float alpha) const;
    ExtremeSearcher* getSearcher(SearchTarget target);
    FloatCoordinate updateAverage(FloatCoordinate coordinate);
    void detectPeaks(FloatCoordinate sample);
    static float highPassFilter(float measure, float previous, float filterValue, float alpha);
    static float lowPassFilter(float measure, float filterValue, float alpha);
    void markAnomalies();
    void resetAnomalies();
    void resetFilters(FloatCoordinate initialSample);
};

#endif
