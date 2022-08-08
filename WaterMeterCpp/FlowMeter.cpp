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

// initialization values for the searcher
constexpr int MIN_SENSOR_VALUE = -4096;
constexpr int MAX_SENSOR_VALUE = 4096;

// --- Constructors and public methods ---

FlowMeter::FlowMeter(EventServer* eventServer):
    EventClient(eventServer),
    _maxXSearcher(MaxX, { MIN_SENSOR_VALUE, 0 }, & _minYSearcher),
    _maxYSearcher(MaxY, {0, MIN_SENSOR_VALUE}, &_maxXSearcher),
    _minXSearcher(MinX, {MAX_SENSOR_VALUE, 0}, &_maxYSearcher),
    _minYSearcher(MinY, { 0, MAX_SENSOR_VALUE }, & _minXSearcher),
    _outlier(eventServer, Topic::Exclude),
    _pulse(eventServer, Topic::Pulse) {}

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
    // If we have an amplitude larger than that, it is an outlier
    _outlierThreshold = OUTLIER_DISTANCE_DIFFERENCE * gain / 1000.0f;
    // We assume X and Y ranges have the same noise levels. The maximum difference in distance between samples
    // where we can still be in the noise range is sqrt(maxXdistance^2 + maxYdistance^2)
    _maxNoiseDistance = sqrtf(static_cast<float>(2 * noiseRange * noiseRange));
    _eventServer->subscribe(this, Topic::Sample);
    _eventServer->subscribe(this, Topic::SensorWasReset);
}

FloatCoordinate FlowMeter::currentExtreme() const {
    if (_searchTarget == None) return FloatCoordinate{};
    return _currentSearcher->extreme();
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
        _averageStartValue = _smooth;
        _firstRound = false;
        updateAverage(_smooth);
        return;
    }
    // we are beyond the first run. Moving average buffer is filled.
    _smooth = lowPassFilter(movingAverage(), _smooth, LOW_PASS_ALPHA_XY);
    if (!_flowStarted) {

        // We need to wait while we are not moving, as we don't know where we are in the ellipse.
        // Move is determined via the distance between the initial sample and the current sample:
        // if that is beyond noise range we have flow. We don't use smoothened values here as we
        // don't want to miss drift.
        const auto distanceFromStart = sample.distanceFrom(_firstSample);
        if (distanceFromStart <= _maxNoiseDistance) {
            // We're still waiting. As noise can have considerable impact, take the average value
            // of the filtered measurements to get a reasonable starting value for when flow starts.
            updateAverage(_smooth);
            _flowThresholdPassedCount = 0;
            return;
        }
        _flowThresholdPassedCount++;
        // We now established there is flow. We now need to find the direction so we can get the next extreme target
        // To make sure we have a good sense of direction of the signal (and eliminate potential AC interference), we wait
        // until the distance between the average starting value and the filtered value is beyond the noise range.
        const auto distanceFromAverageStart = _smooth.distanceFrom(_averageStartValue);
        if (distanceFromAverageStart < _maxNoiseDistance) return;
        _flowStarted = true;
        // The difference between the start point and the current point tells us the direction
        // which in turn tells us the next extreme (state) we're looking for.

        const auto differenceWithStart = _smooth.differenceWith(_averageStartValue);
        _searchTarget = getTarget(differenceWithStart);
        _currentSearcher = getSearcher(_searchTarget);
        _currentSearcher->begin(_maxNoiseDistance);
    }

    // Now we are in an eternal loop of finding an extreme, sending a pulse, and moving to the next.
    // The current searcher cannot be nullptr, so this is safe

    _currentSearcher->addMeasurement(_smooth);
    _isPulse = _currentSearcher->foundExtreme();
    if (_isPulse) {
        _eventServer->publish(Topic::Pulse, _searchTarget);
        _currentSearcher = _currentSearcher->next();
        _searchTarget = _currentSearcher->target();
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

// The signs of the differences tells us where we are on the ellipse, and what
// the next extreme is that we should encounter: if the X and Y differences are both positive we move
// towards the maximum Y value, if X is positive and Y is negative, we move to the maximum X value, and so on.

SearchTarget FlowMeter::getTarget(const FloatCoordinate direction) {
    if (direction.x > 0) {
        if (direction.y > 0) return MaxY;
        return MaxX;
    }
    if (direction.y > 0) return MinX;
    return MinY;
}

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
    _firstSample = initialSample;
    _firstRound = true;
    _movingAverageIndex = 0;
    updateMovingAverage(initialSample);
    _flowThresholdPassedCount = 0;
    _averageStartValue = {0, 0};
    _averageCount = 0;
    _searchTarget = None;
    _isPulse = false;

    _smooth = {0, 0};
    _averageAbsoluteDistance = initialSample.distanceFrom({{0, 0}});
}

FloatCoordinate FlowMeter::updateAverage(const FloatCoordinate coordinate) {
    _averageStartValue.x = _averageStartValue.x * _averageCount + coordinate.x;
    _averageStartValue.y = _averageStartValue.y * _averageCount + coordinate.y;
    _averageCount++;
    _averageStartValue.x /= _averageCount;
    _averageStartValue.y /= _averageCount;
    return _averageStartValue;
}

void FlowMeter::updateMovingAverage(const Coordinate sample) {
    _movingAverage[_movingAverageIndex] = sample;
    ++_movingAverageIndex %= 4;
}