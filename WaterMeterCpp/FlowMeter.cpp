// Copyright 2021 Rik Essenius
// 
//   Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//   except in compliance with the License. You may obtain a copy of the License at
// 
//       http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software distributed under the License
//    is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and limitations under the License.

#include "FlowMeter.h"
#include <math.h>

// Wee need the line smooth enough to eliminate noise. Averaging over 20 seems to work OK
constexpr float LOW_PASS_ALPHA = 0.05f;

// Empirically found that this gives a reasonably accurate derivative
constexpr float HIGH_PASS_ALPHA = 0.8f;
// Eliminate much of the noise on the derivative
constexpr float LOW_PASS_ON_HIGH_PASS_ALPHA = 0.05f;

// If the amplitude ls larger than this, it's very likely we have an outlier
constexpr float OUTLIER_THRESHOLD = 200.0f;

// The troughs are usually around -8. Take 25% of that for the zero check, to eliminate much of the noise
constexpr float ZEROCHECK_THESHOLD = -2.0f;

void FlowMeter::addMeasurement(int measurement) {
    bool firstCall = _startupSamplesLeft == STARTUP_SAMPLES;
    if (_startupSamplesLeft > 0) {
        _startupSamplesLeft--;
    }
    // if we don't have a previous measurement yet, use defaults.
    if (firstCall) {
        // since the filters need initial values, set those. Also initialize the anomaly indicators.
        resetAnomalies();
        resetFilters(measurement);
        return;
    }
    detectOutlier(measurement);
    markAnomalies(measurement);
    detectPeaks(measurement);
}

bool FlowMeter::areAllExcluded() {
    return _excludeAll;
}

void FlowMeter::detectOutlier(int measurement) {
    float amplitude = fabsf(_smoothValue - measurement);
    bool previousIsOutlier = _outlier;
    _outlier = amplitude > OUTLIER_THRESHOLD;
    _firstOutlier = _outlier && !previousIsOutlier;
}

void FlowMeter::detectPeaks(int measurement) {
    if (_outlier) {
      return;
    }
    // smoothen the measurement
    _smoothValue = lowPassFilter((float)measurement, _smoothValue, LOW_PASS_ALPHA);
    // from this we want the peaks. Approximate the derivative via a high pass filter
    _derivative = highPassFilter(_smoothValue, _previousSmoothValue, _derivative, HIGH_PASS_ALPHA);
    // smoothen the derivative to minimize noise
    _smoothDerivative = lowPassFilter(_derivative, _smoothDerivative, LOW_PASS_ON_HIGH_PASS_ALPHA);
    // when the derivative moves from positive to negative (with a threshold to eliminate noise) we have a peak in the original signal
    _peak = _smoothDerivative <= ZEROCHECK_THESHOLD && _previousSmoothDerivative > ZEROCHECK_THESHOLD ? 1 : 0;
    _previousSmoothValue = _smoothValue;
    _previousSmoothDerivative = _smoothDerivative;
}

float FlowMeter::getSmoothValue() {
  return _smoothValue;
}

float FlowMeter::getDerivative() {
  return _derivative;
}

float FlowMeter::getSmoothDerivative() {
  return _smoothDerivative;
}

bool FlowMeter::isPeak() {
    return _peak;
}

float FlowMeter::highPassFilter(float measure, float previous, float filterValue, float alpha) {
    return  alpha * (filterValue + measure - previous);
}

bool FlowMeter::isExcluded() {
    return _exclude;
}

bool FlowMeter::isOutlier() {
    return _outlier;
}

float FlowMeter::lowPassFilter(float measure, float filterValue, float alpha) {
    return alpha * measure + (1 - alpha) * filterValue;
}

void FlowMeter::markAnomalies(int measurement) {
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
    _firstOutlier = false;
    _exclude = false;  
    _excludeAll = false;
}

void FlowMeter::resetFilters(int initialMeasurement) {
    _smoothValue = (float)initialMeasurement;
    _derivative = 0.0f;
    _smoothDerivative = 0.0f;
    _minDerivative = MIN_DERIVATIVE_PEAK;
    _previousSmoothValue = _smoothValue;
}
