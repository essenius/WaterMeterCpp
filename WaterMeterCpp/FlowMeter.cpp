// Copyright 2021-2022 Rik Essenius
// 
//   Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//   except in compliance with the License. You may obtain a copy of the License at
// 
//       http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software distributed under the License
//    is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and limitations under the License.


// TODO: handle state in which sensor doesn't return new data anymore. Nuclear option is rebooting.

#include "FlowMeter.h"
#include <cmath>

#include "EventServer.h"

// if the smooth absolute of the derivative is larger than the threshold, we have flow.
constexpr float FLOW_THRESHOLD = 1.0f;

// Empirically found that this gives a reasonably accurate derivative
constexpr float HIGH_PASS_ALPHA = 0.8f;

// We need the line smooth enough to eliminate noise. Averaging over 20 seems to work OK
constexpr float LOW_PASS_ALPHA = 0.05f;

// Eliminate much of the noise on the derivative
constexpr float LOW_PASS_ON_HIGH_PASS_ALPHA = 0.05f;

// If the amplitude ls larger than this, it's very likely we have an outlier
constexpr float OUTLIER_THRESHOLD = 200.0f;

// The troughs are usually around -8. Take 25% of that for the zero check, to eliminate much of the noise
constexpr float ZEROCHECK_THESHOLD = -2.0f;

FlowMeter::FlowMeter(EventServer* eventServer):
    EventClient(eventServer),
    _exclude(eventServer, Topic::Exclude),
    _flow(eventServer, Topic::Flow),
    _peak(eventServer, Topic::Peak) {}

void FlowMeter::addSample(const int measurement) {
    _firstCall = _startupSamplesLeft == STARTUP_SAMPLES;
    if (_startupSamplesLeft > 0) {
        _startupSamplesLeft--;
    }
    // if we don't have a previous measurement yet, use defaults.
    if (_firstCall) {
        // since the filters need initial values, set those. Also initialize the anomaly indicators.
        resetAnomalies();
        resetFilters(measurement);
        return;
    }
    detectOutlier(measurement);
    markAnomalies(measurement);
    detectPeaks(measurement);
}

bool FlowMeter::areAllExcluded() const {
    return _excludeAll;
}

void FlowMeter::begin() {
    _eventServer->subscribe(this, Topic::Sample);
    _eventServer->subscribe(this, Topic::SensorWasReset);
}

void FlowMeter::detectOutlier(const int measurement) {
    const float amplitude = fabsf(_smoothValue - static_cast<float>(measurement));
    _outlier = amplitude > OUTLIER_THRESHOLD;
}

void FlowMeter::detectPeaks(const int measurement) {
    if (_outlier) {
        return;
    }
    // smoothen the measurement
    _smoothValue = lowPassFilter(static_cast<float>(measurement), _smoothValue, LOW_PASS_ALPHA);
    // from this we want the peaks. Approximate the derivative via a high pass filter
    _derivative = highPassFilter(_smoothValue, _previousSmoothValue, _derivative, HIGH_PASS_ALPHA);
    // smoothen the derivative to minimize noise
    _smoothDerivative = lowPassFilter(_derivative, _smoothDerivative, LOW_PASS_ON_HIGH_PASS_ALPHA);
    // when the derivative moves from positive to negative (with a threshold to eliminate noise) we have a peak in the original signal
    _peak = _smoothDerivative <= ZEROCHECK_THESHOLD && _previousSmoothDerivative > ZEROCHECK_THESHOLD;

    // we use the smooth abs derivative to check for flow
    _smoothAbsDerivative = lowPassFilter(fabsf(_derivative), _smoothAbsDerivative, LOW_PASS_ON_HIGH_PASS_ALPHA);
    _flow = _smoothAbsDerivative > FLOW_THRESHOLD;

    _previousSmoothValue = _smoothValue;
    _previousSmoothDerivative = _smoothDerivative;
}

float FlowMeter::getDerivative() const {
    return _derivative;
}

float FlowMeter::getSmoothAbsDerivative() const {
    return _smoothAbsDerivative;
}

float FlowMeter::getSmoothDerivative() const {
    return _smoothDerivative;
}

float FlowMeter::getSmoothValue() const {
    return _smoothValue;
}

bool FlowMeter::hasFlow() const {
    return _flow;
}

float FlowMeter::highPassFilter(const float measure, const float previous, const float filterValue, const float alpha) {
    return alpha * (filterValue + measure - previous);
}

bool FlowMeter::isExcluded() const {
    return _exclude;
}

bool FlowMeter::isOutlier() const {
    return _outlier;
}

bool FlowMeter::isPeak() const {
    return _peak;
}

float FlowMeter::lowPassFilter(const float measure, const float filterValue, const float alpha) {
    return alpha * measure + (1 - alpha) * filterValue;
}

void FlowMeter::markAnomalies(const int measurement) {
    _exclude = _outlier;
    if (!_exclude) {
        return;
    }
    _excludeAll = _startupSamplesLeft > 0;
    if (!_excludeAll) {
        return;
    }

    // We have a problem in the first few measurements. It might as well have been the first one (e.g. startup issue).
    // so we discard what we have so far and start again. We keep the current value as seed for the low pass filters.
    // If this was the outlier, it will be caught the next time.
    resetFilters(measurement);
    _startupSamplesLeft = STARTUP_SAMPLES;
}

void FlowMeter::resetAnomalies() {
    _outlier = false;
    _flow = false;
    _exclude = false;
    _excludeAll = false;
}

void FlowMeter::resetFilters(const int initialMeasurement) {
    _smoothValue = static_cast<float>(initialMeasurement);
    _derivative = 0.0f;
    _smoothDerivative = 0.0f;
    _smoothAbsDerivative = 0.0f;
    _minDerivative = MIN_DERIVATIVE_PEAK;
    _previousSmoothValue = _smoothValue;
}

void FlowMeter::update(const Topic topic, const long payload) {
    if (topic == Topic::Sample) {
        addSample(static_cast<int>(payload));
    } else if (topic == Topic::SensorWasReset) {
        // If we needed to reset the sensor, also reset the measurement process when the next sample comes in
       _startupSamplesLeft = STARTUP_SAMPLES;
    }
}

bool FlowMeter::wasReset() const {
    return _firstCall;
}
