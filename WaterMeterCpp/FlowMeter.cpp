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

static const float DRIFT_THRESHOLD_IF_IDLE = 7.5f;
static const float DRIFT_THRESHOLD_IN_FLOW = 7.5f;
const float DERIVATIVE_HIGH_PASS_ALPHA = 0.5f;
const float HIGH_PASS_ALPHA = 0.5f;
const float LOW_PASS_AMPLITUDE_ALPHA = 0.05f;
const float LOW_PASS_ALPHA_FAST = 0.1f;
const float LOW_PASS_ALPHA_SLOW = 0.03f;
const float LOW_PASS_ON_DERIVATIVE_AMPLITUDE_ALPHA = 0.1;
const float LOW_PASS_ON_DIFFERENCE_ALPHA = 0.1f;
const float LOW_PASS_ON_HIGH_PASS_ALPHA = 0.2f;

const float OUTLIER_THRESHOLD = 200.0f;
const int PEAK_HORIZON = 10;

const float SWITCH_FLOW_OFF_THRESHOLD = 6.0f;
const float SWITCH_FLOW_ON_THRESHOLD = 10.0f;

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
        _amplitude = 0;
        return;
    }
    detectOutlier(measurement);
    if (!_firstOutlier) {
        detectFlow(measurement);
        detectDrift(measurement);
    }
    markAnomalies(measurement);
    detectPeaks();
}

bool FlowMeter::areAllExcluded() {
    return _excludeAll;
}

float FlowMeter::getAmplitude() {
    return _amplitude;
}

float FlowMeter::getLowPassFast() {
    return _lowPassFast;
}

float FlowMeter::getLowPassSlow() {
    return _lowPassSlow;
}

float FlowMeter::getLowPassOnHighPass() {
    return _lowPassOnHighPass;
}

bool FlowMeter::hasDrift() {
    return _drift;
}

bool FlowMeter::hasFlow() {
    return _flow;
}

int FlowMeter::getPeak() {
    return _peak;
}

bool FlowMeter::isExcluded() {
    return _exclude;
}

bool FlowMeter::isOutlier() {
    return _outlier;
}

void FlowMeter::detectDrift(int measurement) {
    _lowPassSlow = lowPassFilter(measurement, _lowPassSlow, LOW_PASS_ALPHA_SLOW);
    _lowPassFast = lowPassFilter(measurement, _lowPassFast, LOW_PASS_ALPHA_FAST);
    _lowPassDifference = fabsf(_lowPassSlow - _lowPassFast);
    _lowPassOnDifference = lowPassFilter(_lowPassDifference, _lowPassOnDifference, LOW_PASS_ON_DIFFERENCE_ALPHA);

    // this works on the flow of the previous sample (this is why we have CalculatedFlow).
    _drift = _lowPassOnDifference >= (_flow ? DRIFT_THRESHOLD_IN_FLOW : DRIFT_THRESHOLD_IF_IDLE);
}

void FlowMeter::detectFlow(int measurement) {
    if (_outlier) {
        _calculatedFlow = false;
        return;
    }
    _highPass = highPassFilter(measurement, _previousMeasure, _highPass, HIGH_PASS_ALPHA);
    _lowPassOnHighPass = lowPassFilter(fabs(_highPass), _lowPassOnHighPass, LOW_PASS_ON_HIGH_PASS_ALPHA);
    _calculatedFlow = _lowPassOnHighPass >= (_flow ? SWITCH_FLOW_OFF_THRESHOLD : SWITCH_FLOW_ON_THRESHOLD);
    _previousMeasure = measurement;
}

void FlowMeter::detectOutlier(int measurement) {
    _amplitude = fabsf(_lowPassSlow - measurement);
    bool previousIsOutlier = _outlier;
    _outlier = _amplitude > OUTLIER_THRESHOLD;
    _firstOutlier = _outlier && !previousIsOutlier;
}

float FlowMeter::sign(float value) {
    return (value > 0) ? 1 : ((value < 0) ? -1 : 0);
}

void FlowMeter::detectPeaks() {
    float previousLowPassAmplitude = _lowPassAmplitude;
    // Amplitude was already calculated in detectOutlier
    _lowPassAmplitude = lowPassFilter(_amplitude, _lowPassAmplitude, LOW_PASS_AMPLITUDE_ALPHA);
    _derivativeAmplitude = highPassFilter(_lowPassAmplitude, previousLowPassAmplitude, _derivativeAmplitude, DERIVATIVE_HIGH_PASS_ALPHA);
    float previousLowPassOnDerivativeAmplitude = _lowPassOnDerivativeAmplitude;
    _lowPassOnDerivativeAmplitude = lowPassFilter(_derivativeAmplitude, _lowPassOnDerivativeAmplitude, LOW_PASS_ON_DERIVATIVE_AMPLITUDE_ALPHA);

    // We don't look for peaks when we are not in flow.
    if (!_flow) return;

    // we have a peak if we move from negative to positive or vice versa
    _calculatedPeak = _lowPassOnDerivativeAmplitude < 0 && previousLowPassOnDerivativeAmplitude > 0 ? 1
        : _lowPassOnDerivativeAmplitude > 0 && previousLowPassOnDerivativeAmplitude < 0 ? -1 : 0;
    // We can have a couple of small peaks close to 0 and we need to filter those out. We want just one within the horizon,
    // unless we move back in the same direction we came from. Then we had another peak (which we introduce slightly late).
    _peakCountDown = _peakCountDown != 0 ? _peakCountDown - 1 : _calculatedPeak != 0 ? PEAK_HORIZON : 0;
    switch (_peakCountDown) {
    case PEAK_HORIZON:
        _lastValueBeforePeak = previousLowPassOnDerivativeAmplitude;
        _peak = _calculatedPeak;
        break;
    case 1:
        // if we have a pos/neg or neg/pos change, we have had another peak. Should not occur too often, but often enough to cater for.
        _peak = sign(_lastValueBeforePeak) == sign(_lowPassOnDerivativeAmplitude) ? -sign(_lastValueBeforePeak) : 0;
        break;
    default:
        _peak = 0;
    }
}

float FlowMeter::highPassFilter(float measure, float previous, float filterValue, float alpha) {
    return  alpha * (filterValue + measure - previous);
}

float FlowMeter::lowPassFilter(float measure, float filterValue, float alpha) {
    return alpha * measure + (1 - alpha) * filterValue;
}

void FlowMeter::markAnomalies(int measurement) {
    _exclude = _outlier || _drift;
    _flow = _calculatedFlow && !_exclude;
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
    _calculatedFlow = false;
    _drift = false;
    _exclude = false;
    _excludeAll = false;
    _flow = false;
    _outlier = false;
    _firstOutlier = false;
}

void FlowMeter::resetFilters(int initialMeasurement) {
    _highPass = 0.0f;
    _lowPassFast = (float)initialMeasurement;
    _lowPassDifference = 0.0f;
    _lowPassOnDifference = 0.0f;
    _lowPassOnHighPass = 0.0f;
    _lowPassSlow = (float)initialMeasurement;
    _previousMeasure = (float)initialMeasurement;
}
