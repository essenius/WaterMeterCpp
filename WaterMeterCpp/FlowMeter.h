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

class FlowMeter : public EventClient {
public:
    // if we have more than this number of outliers in a row, we reset the sensor
    static constexpr unsigned int MAX_CONSECUTIVE_OUTLIERS = 50;
    explicit FlowMeter(EventServer* eventServer);
    void addSample(Coordinate sample);
    void begin(int noiseRange, float gain);
    FloatCoordinate currentExtreme() const;
    bool isOutlier() const { return _outlier; }
    bool isPulse() const { return _isPulse; }
    SearchTarget searchTarget() const { return _searchTarget; }

    void update(Topic topic, long payload) override;
    void update(Topic topic, Coordinate payload) override;
    bool wasReset() const { return _firstCall; }

    static constexpr int SAMPLE_PERIOD_MICROS = 10000;
    static constexpr float SAMPLE_PERIOD_SECONDS = SAMPLE_PERIOD_MICROS / 1000000.0f;

    FloatCoordinate getSmoothSample() const { return _smooth; }

protected:
    static constexpr int8_t MOVING_AVERAGE_BUFFER_SIZE = 4;
    float _averageAbsoluteDistance = 0.0f;
    float _averageCount = 0.0f;
    FloatCoordinate _averageStartValue = {};
    ExtremeSearcher* _currentSearcher = nullptr;
    unsigned int _consecutiveOutliers = 0;
    Coordinate _firstSample{};
    bool _firstCall = true;
    bool _firstRound = true;
    bool _flowStarted = false;
    int8_t _flowThresholdPassedCount = 0;
    bool _isPulse = false;
    // will be overwritten in begin()
    float _maxNoiseDistance = 0;
    ExtremeSearcher _maxXSearcher;
    ExtremeSearcher _maxYSearcher;
    ExtremeSearcher _minXSearcher;
    ExtremeSearcher _minYSearcher;
    Coordinate _movingAverage[MOVING_AVERAGE_BUFFER_SIZE] = {};
    int8_t _movingAverageIndex = 0;
    int8_t _movingAverageIndexStartupLeft = MOVING_AVERAGE_BUFFER_SIZE - 1;
    ChangePublisher<bool> _outlier;
    // will be overwritten in begin()
    float _outlierThreshold = 0.0f;
    ChangePublisher<int> _pulse;
    SearchTarget _searchTarget = None;
    FloatCoordinate _smooth = {0.0f, 0.0f};

    void detectOutlier(Coordinate measurement);
    void detectPulse(Coordinate sample);
    ExtremeSearcher* getSearcher(SearchTarget target);
    static SearchTarget getTarget(FloatCoordinate direction);
    static float lowPassFilter(float measure, float filterValue, float alpha);
    FloatCoordinate lowPassFilter(FloatCoordinate measure, FloatCoordinate filterValue, float alpha) const;
    void markAnomalies();
    FloatCoordinate movingAverage() const;
    void resetAnomalies();
    void resetFilters(Coordinate initialSample);
    FloatCoordinate updateAverage(FloatCoordinate coordinate);
    void updateMovingAverage(Coordinate sample);
};

#endif
