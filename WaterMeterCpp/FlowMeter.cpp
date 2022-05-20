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

// most signals are between 1 Hz and 8 Hz. We call that fast. But smetimes with very slow flows (e.g. someone
// didn't close the tap completely) slower frequencies  occur, and the filter would suppress those.
// That's why we also use a slower filter, checking for 0.05 Hz to 1 Hz. We call that slow.

// Fast cut-off frequency 8 Hz. Alpha for low pass = dt/(1/(2*pi*fc)+dt), dt = 0.01s
constexpr float ALPHA_LOW_PASS_FAST =0.335f;
// Slow cut-off frequency 1 Hz
constexpr float ALPHA_LOW_PASS_SLOW = 0.0591f;

// Fast cut-off frequency 1 Hz
// alpha for high pass = 1/(2*pi*dt*fc+1)
constexpr float ALPHA_HIGH_PASS_FAST = 0.941f;
// slow cut-off frequncy 0.05 Hz
constexpr float ALPHA_HIGH_PASS_SLOW = 0.997f;

// take the highest measured range times 2 as outlier limit
// If the amplitude ls larger than this, it's very likely we have an outlier
constexpr float OUTLIER_AMPLITUDE_MILLIGAUSS = 53.0f * 2.0f;

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

float FlowMeter::getAmplitude() const {
    return _amplitude;
}

void FlowMeter::begin(const int noiseRange, const float gain) {

    if (noiseRange > 30) {
        // We have a QMC sensor, which is more sensitive and has higher noise range.
        // We only use the 8 Gauss range, and zero threshold was empirically determined via null measurements
        _zeroThreshold = 12.0f;
    } else {
        // We have an HMC sensor with much lower noise ranges in the different measurement ranges.
        // Emprirically determined a linear relation between range and maximum noise in the filtered signal.
        // We take a margin of 50% to be sure. This should come down to ~0.86 for the 4.7 Gauss range
        constexpr float NOISE_FIT_A = 0.1088f;
        constexpr float NOISE_FIT_B = 0.1384f;
        constexpr float THRESHOLD_MARGIN = 0.5f;
        _zeroThreshold = (NOISE_FIT_A * static_cast<float>(noiseRange) + NOISE_FIT_B) * (1 + THRESHOLD_MARGIN);
    }
    _flowThreshold = _zeroThreshold;
    // Calculate LSB corresponding to the largest measured magnetic field with margin.
    // If we have an amplitude larger than that, it is an outlier
    _outlierThreshold = OUTLIER_AMPLITUDE_MILLIGAUSS * gain / 1000.0f;
    _eventServer->subscribe(this, Topic::Sample);
    _eventServer->subscribe(this, Topic::SensorWasReset);
}

void FlowMeter::detectOutlier(const int measurement) {
    const float amplitude = fabsf(_slowSmooth - static_cast<float>(measurement));
    _outlier = amplitude > _outlierThreshold;
}

void FlowMeter::detectPeaks(const int measurement) {
    // ignore outliers
    if (_outlier) {
        return;
    }
    const auto sample = static_cast<float>(measurement);
    _fastSmooth = lowPassFilter(sample, _fastSmooth, ALPHA_LOW_PASS_FAST);
    // first we do a high pass to center values around zero and approximate a derivative-like function so we can find peaks
    _fastDerivative = highPassFilter(_fastSmooth, _previousFastSmooth, _fastDerivative, ALPHA_HIGH_PASS_FAST);
    // on that we apply a low pass filter
    _smoothFastDerivative = lowPassFilter(_fastDerivative, _smoothFastDerivative, ALPHA_LOW_PASS_FAST);

     // we take a low pass filter on the absolute value to be able to see if we have flow (i.e. a sinusoid signal). 
    _smoothAbsFastDerivative = lowPassFilter(fabsf(_smoothFastDerivative), _smoothAbsFastDerivative, ALPHA_LOW_PASS_SLOW);
    _fastFlow = _smoothAbsFastDerivative > _flowThreshold;

    _slowSmooth = lowPassFilter(sample, _slowSmooth, ALPHA_LOW_PASS_SLOW);
    _amplitude = fabsf(_slowSmooth - sample);
    // If we already have a flow, take the fast signal. If not, calculate the slow signal and take that.
    // this can result in a bit weirdly shaped output signals, but the important bit is the move from positive to negative.
    _combinedDerivative = _fastFlow ? _smoothFastDerivative : highPassFilter(_slowSmooth, _previousSlowSmooth, _combinedDerivative, ALPHA_HIGH_PASS_SLOW);
    _smoothAbsCombinedDerivative = _fastFlow
             ? _smoothAbsFastDerivative
             : lowPassFilter(fabsf(_combinedDerivative), _smoothAbsCombinedDerivative, ALPHA_LOW_PASS_SLOW);
    _flow = _smoothAbsCombinedDerivative > _flowThreshold;

    // To find the number of peaks in the original signal, we look for a move from positive to negative in this derivative signal.
    // It is still a bit noisy, so we do that in two steps. We define a band that we consider to be indistinguishable from zero,
    // and we have a peak if the signal has entered and exited that band with a downward slope.
    _hasEnteredBand |= _previousCombinedDerivative > _zeroThreshold && _combinedDerivative <= _zeroThreshold;
    const bool hasExitedBand = _previousCombinedDerivative >= -_zeroThreshold && _combinedDerivative < -_zeroThreshold;
    _peak = _hasEnteredBand && hasExitedBand;
    if (_peak) {
        _hasEnteredBand = false;
    }

    _previousFastSmooth = _fastSmooth;
    _previousSlowSmooth = _slowSmooth;
    _previousCombinedDerivative = _combinedDerivative;
}

float FlowMeter::getCombinedDerivative() const {
    return _combinedDerivative;
}

float FlowMeter::getFastDerivative() const {
    return _fastDerivative;
}

float FlowMeter::getFastSmoothValue() const {
    return _fastSmooth;
}

float FlowMeter::getSmoothAbsCombinedDerivative() const {
    return _smoothAbsCombinedDerivative;
}

float FlowMeter::getSmoothAbsFastDerivative() const {
    return _smoothAbsFastDerivative;
}

float FlowMeter::getSmoothFastDerivative() const {
    return _smoothFastDerivative;
}

float FlowMeter::getSlowSmoothValue() const {
    return _slowSmooth;
}

float FlowMeter::getZeroThreshold() const {
    return _zeroThreshold;
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

bool FlowMeter::hasEnteredBand() const {
    return _hasEnteredBand;
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
    _fastSmooth = static_cast<float>(initialMeasurement);
    _previousFastSmooth = _fastSmooth;
    _slowSmooth = _fastSmooth;
    _previousSlowSmooth = _fastSmooth;
    _fastDerivative = 0.0f;
    _smoothFastDerivative = 0.0f;
    _smoothAbsFastDerivative = 0.0f;
    _combinedDerivative = 0.0f;
    _previousCombinedDerivative = 0.0f;
    _smoothAbsCombinedDerivative = 0.0F;
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
