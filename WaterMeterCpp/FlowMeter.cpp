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

#include <cmath>

#include "FlowMeter.h"

#include "Angle.h"
#include "EventServer.h"

// It was empirically established that relative measurements are less reliable (losing slow moves in the noise),
// so now using absolutes measurements. Since absolute values are not entirely predictable, the tactic is to
// follow the clockwise elliptic move that the signal makes, and tracking high Y, high X, low Y and low X,
// respectively. The pattern is a state machine.

// To eliminate the noise caused by eletric devices (water pump) we use a sampling rate of 100Hz, which is twice
// the rate frequency of electricity in Europe. Then we take a moving average over 4 measurements.
// Over that signal we do another fast smoothening.

// light smoothening, cut-off frequency (Fc) 16 Hz. Alpha for low pass = dt / (1 / (2 * pi * fc) + dt), with dt = 0.01s
constexpr float LOW_PASS_ALPHA_XY = 0.5f;

// this should be a lot slower (cut-off around 0.84 Hz). We want it to incorporate drifts/shifts, but be reasonably stable
constexpr float LOW_PASS_ALPHA_ABSOLUTE_DISTANCE = 0.05f;

// The largest range in distance difference observed in the test data;
constexpr float DISTANCE_RANGE_MILLIGAUSS = 120.0f;
constexpr float OUTLIER_DISTANCE_DIFFERENCE = 2.0f * DISTANCE_RANGE_MILLIGAUSS;


// --- Constructors and public methods ---

FlowMeter::FlowMeter(EventServer* eventServer) :
    EventClient(eventServer),
    _noneSearcher(None, {ExtremeSearcher::MIN_SENSOR_VALUE, ExtremeSearcher:: MIN_SENSOR_VALUE}, nullptr),
    _maxXSearcher(MaxX, {ExtremeSearcher::MIN_SENSOR_VALUE, 0 }, & _minYSearcher),
    _maxYSearcher(MaxY, { 0, ExtremeSearcher::MIN_SENSOR_VALUE }, & _maxXSearcher),
    _minXSearcher(MinX, {ExtremeSearcher::MAX_SENSOR_VALUE, 0 }, & _maxYSearcher),
    _minYSearcher(MinY, { 0, ExtremeSearcher::MAX_SENSOR_VALUE }, & _minXSearcher),
    _outlier(eventServer, Topic::Exclude) {}

void FlowMeter::addSample(const Coordinate sample) {

    // if we don't have a previous measurement yet, use defaults.
    if (_firstCall) {
        // since the filters need initial values, set those. Also initialize the anomaly indicators.
        resetAnomalies();
        resetFilters(sample);
        _firstCall = false;
        return;
    }
    detectOutlier(sample);
    markAnomalies();
    detectPulse(sample);
}

void FlowMeter::begin(const int noiseRange, const float gain) {

    // Calculate LSB corresponding to the largest measured magnetic field with margin.
    // If we have a distance difference larger than that, it is an outlier
    _outlierThreshold = OUTLIER_DISTANCE_DIFFERENCE * gain / 1000.0f;
    // We assume X and Y ranges have the same noise levels. The maximum difference in distance between samples
    // where we can still be in the noise range is sqrt(maxXdistance^2 + maxYdistance^2)

    const auto noiseBase = static_cast<float>(noiseRange);
    _maxNoiseDistance = sqrtf(2.0f * noiseBase * noiseBase);
    _eventServer->subscribe(this, Topic::Sample);
    _eventServer->subscribe(this, Topic::SensorWasReset);
}

FloatCoordinate FlowMeter::currentExtreme() const {
    if (_currentSearcher == nullptr) return FloatCoordinate{};
    return _currentSearcher->extreme();
}

SearchTarget FlowMeter::searchTarget() const {
    if (_currentSearcher == nullptr) return None;
    return _currentSearcher->target();
}

void FlowMeter::update(const Topic topic, const long payload) {
    if (topic == Topic::SensorWasReset) {
        // If we needed to begin the sensor, also begin the measurement process when the next sample comes in
        _firstCall = true;
        _consecutiveOutliers = 0;
    }
}

void FlowMeter::update(const Topic topic, const Coordinate payload) {
    if (topic == Topic::Sample) {
        addSample(payload);
    }
}

// --- Protected methods ---

void FlowMeter::detectOutlier(const Coordinate measurement) {
    const float distance = measurement.distanceFrom({ { 0,0 } });
    _outlier = fabsf(distance - _averageAbsoluteDistance) > _outlierThreshold;
    if (_outlier) return;
    _averageAbsoluteDistance = lowPassFilter(distance, _averageAbsoluteDistance, LOW_PASS_ALPHA_ABSOLUTE_DISTANCE);
}

void FlowMeter::detectPulse(const Coordinate sample) {
    // ignore outliers
    if (_outlier) {
        return;
    }
    updateMovingAverage(sample);
    if (_firstRound) {
        // if index is 0, we made the first round and the buffer is full. Otherwise we wait.
        if (_movingAverageIndex != 0) return;
        // Initialize the filter for the next run
        _smooth = movingAverage();
        _smoothStartValue = _smooth;
        _firstRound = false;
        _noneSearcher.begin(_maxNoiseDistance);
        _noneSearcher.addMeasurement(_smoothStartValue);
        return;
    }
    // we are beyond the first run. Moving average buffer is filled.
    _smooth = lowPassFilter(movingAverage(), _smooth, LOW_PASS_ALPHA_XY);
    if (!_flowStarted) {

        // We need to wait while we are not moving, as we don't know where we are in the ellipse.
        // Move is determined via two factors:
        // * the distance between the initial sample and the current sample must be beyond noise threshold
        // * the angle difference must be small enough (i.e not random)

        _noneSearcher.addMeasurement(_smooth);
        _flowStarted = _noneSearcher.foundTarget();
        if (!_flowStarted) return;
        _currentSearcher = getSearcher(_noneSearcher.target());
        _currentSearcher->begin(_maxNoiseDistance);
    }

    // Now we are in an eternal loop of finding an extreme, sending a pulse, and moving to the next.
    // The current searcher cannot be nullptr, so this is safe

    _currentSearcher->addMeasurement(_smooth);
    _isPulse = _currentSearcher->foundTarget();
    if (_isPulse) {
        _eventServer->publish(Topic::Pulse, _currentSearcher->target());
        _currentSearcher = _currentSearcher->next();
    }
}

ExtremeSearcher* FlowMeter::getSearcher(const SearchTarget target) {
    switch (target) {
    case MaxY: return &_maxYSearcher;
    case MaxX: return &_maxXSearcher;
    case MinY: return &_minYSearcher;
    case MinX: return &_minXSearcher;
    default:
        return nullptr;
    }
}
// The angle between the X axis and the line through the start value and the current value tells us where we are on the ellipse,
// and what the next extreme is that we should encounter.

float FlowMeter::lowPassFilter(const float measure, const float filterValue, const float alpha) {
    return alpha * measure + (1 - alpha) * filterValue;
}

FloatCoordinate FlowMeter::lowPassFilter(const FloatCoordinate measure, const FloatCoordinate filterValue, const float alpha) const {
    return {
        lowPassFilter(measure.x, filterValue.x, alpha),
        lowPassFilter(measure.y, filterValue.y, alpha)
    };
}

void FlowMeter::markAnomalies() {
    if (_outlier) {
        _consecutiveOutliers++;
        if (_consecutiveOutliers == MAX_CONSECUTIVE_OUTLIERS) {
            _eventServer->publish(Topic::ResetSensor, LONG_TRUE);
        }
        return;
    }
    _consecutiveOutliers = 0;
}

FloatCoordinate FlowMeter::movingAverage() const {
    FloatCoordinate result{};
    for (const auto i : _movingAverage) {
        result.x += static_cast<float>(i.x);
        result.y += static_cast<float>(i.y);
    }
    result.x /= MOVING_AVERAGE_BUFFER_SIZE;
    result.y /= MOVING_AVERAGE_BUFFER_SIZE;
    return result;
}

void FlowMeter::resetAnomalies() {
    _outlier = false;
}

void FlowMeter::resetFilters(const Coordinate initialSample) {
    _flowStarted = false;
    _noneSearcher.begin(_maxNoiseDistance);
    _firstRound = true;
    _movingAverageIndex = 0;
    updateMovingAverage(initialSample);
    _smoothStartValue = {};
    _currentSearcher = nullptr;
    _isPulse = false;

    _smooth = {0, 0};
    _averageAbsoluteDistance = initialSample.distanceFrom({{0, 0}});
}

void FlowMeter::updateMovingAverage(const Coordinate sample) {
    _movingAverage[_movingAverageIndex] = sample;
    ++_movingAverageIndex %= 4;
}