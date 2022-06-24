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

// The largest range in distance difference observed in the test data;
constexpr float DISTANCE_RANGE_MILLIGAUSS = 120.0f;
constexpr float OUTLIER_DISTANCE_DIFFERENCE = 2.0f * DISTANCE_RANGE_MILLIGAUSS;

// if we have more than this number of outliers in a row, we reset the sensor
constexpr unsigned int MAX_CONSECUTIVE_OUTLIERS = 10;

// new
// cut-off frequency 4 Hz. Alpha for low pass = dt / (1 / (2 * pi * fc) + dt), dt = 0.01s
constexpr float LOW_PASS_ALPHA_COORDINATE = 0.2f;

// cut-off frequency 1 Hz. alpha for high pass = 1 / (2 * pi * dt * fc + 1)
constexpr float HIGH_PASS_ALPHA_COORDINATE = 0.941f;

// Requires a bit more smoothing so halving the alpha (cut-off 1.77 Hz)
constexpr float LOW_PASS_ALPHA_DISTANCE = 0.1f;

constexpr float LOW_PASS_ALPHA_ANGLE = 0.1f;

constexpr float LOW_PASS_ALPHA_ABSOLUTE_DISTANCE = 0.05f;


// Calibrated with test data using HMC5883, gain 4.7
// TODO: parameterize for sensor ranges

constexpr float SCORE_THRESHOLD = 1.25f;

constexpr float HIGH_X_LENGTH = 0.24f;
constexpr float LOW_X_LENGTH = 0.1f;
constexpr float A_DISTANCE = 1 / (HIGH_X_LENGTH - LOW_X_LENGTH);
constexpr float B_DISTANCE = -A_DISTANCE * LOW_X_LENGTH;

constexpr float HIGH_X_CORDIF = 0.01f;
constexpr float LOW_X_CORDIF = 0.09f;
constexpr float A_CORDIF = 1 / (HIGH_X_CORDIF - LOW_X_CORDIF);
constexpr float B_CORDIF = -A_CORDIF * LOW_X_CORDIF;

FlowMeter::FlowMeter(EventServer* eventServer):
	EventClient(eventServer),
	_exclude(eventServer, Topic::Exclude),
	_flow(eventServer, Topic::Flow),
	_peak(eventServer, Topic::Peak),
	_cordifLowPass(round(_angle)) {}

float FlowMeter::score(const float input, const float a, const float b) const {
	return std::min(std::max(a * input + b, 0.0f), 1.0f);
}

float FlowMeter::correctedDifference(const float previousAngle, const float currentAngle) {
	const auto difference = currentAngle - previousAngle;
	// the atan2 function returns values between -PI and PI.
	// if there is noise around the extreme values it flips around, giving differences of around 2*PI.
	// As it is not very likely that a normal difference is going to be beyond PI,
	// we assume that if that happens, we have such a flip.
	if (difference > PI) return difference - 2 * PI;
	if (difference < -PI) return difference + 2 * PI;
	return difference;
}

void FlowMeter::addSample(const Coordinate sample) {

	const FloatCoordinate floatSample { static_cast<float>(sample.x), static_cast<float>(sample.y) };

	// if we don't have a previous measurement yet, use defaults.
	if (_firstCall) {
		// since the filters need initial values, set those. Also initialize the anomaly indicators.
		resetAnomalies();
		resetFilters(floatSample);
		_firstCall = false;
		return;
	}
	detectOutlier(floatSample);
	markAnomalies();
	detectPeaks(floatSample);
}

// TODO: replace _zeroThreshold by a/b parameters per range
void FlowMeter::begin(const int noiseRange, const float gain) {

	if (noiseRange > 30) {
		// We have a QMC sensor, which is more sensitive and has higher noise range.
		// We only use the 8 Gauss range, and zero threshold was empirically determined via null measurements
		_zeroThreshold = 12.0f;
	}
	else {
		// We have an HMC sensor with much lower noise ranges in the different measurement ranges.
		// Emprirically determined a linear relation between range and maximum noise in the filtered signal.
		// We take a margin of 50% to be sure. This should come down to ~0.86 for the 4.7 Gauss range
		constexpr float NOISE_FIT_A = 0.1088f;
		constexpr float NOISE_FIT_B = 0.1384f;
		constexpr float THRESHOLD_MARGIN = 0.5f;
		_zeroThreshold = (NOISE_FIT_A * static_cast<float>(noiseRange) + NOISE_FIT_B) * (1 + THRESHOLD_MARGIN);
	}
	// Calculate LSB corresponding to the largest measured magnetic field with margin.
	// If we have an amplitude larger than that, it is an outlier
	_outlierThreshold = OUTLIER_DISTANCE_DIFFERENCE * gain / 1000.0f;
	_eventServer->subscribe(this, Topic::Sample);
	_eventServer->subscribe(this, Topic::SensorWasReset);
}

void FlowMeter::detectOutlier(const FloatCoordinate measurement) {
	const float distance = measurement.distanceFromOrigin();
	_outlier = fabsf(distance - _averageAbsoluteDistance) > _outlierThreshold;
	if (_outlier) return;
	_averageAbsoluteDistance = lowPassFilter(distance, _averageAbsoluteDistance, LOW_PASS_ALPHA_ABSOLUTE_DISTANCE);
}

void FlowMeter::detectPeaks(const FloatCoordinate sample) {
	// ignore outliers
	if (_outlier) {
		return;
	}
	// Reduce the noise in the sensor values a bit.
	_smooth.x = lowPassFilter(sample.x, _smooth.x, LOW_PASS_ALPHA_COORDINATE);
	_smooth.y = lowPassFilter(sample.y, _smooth.y, LOW_PASS_ALPHA_COORDINATE);

	// Apply a high pass filter, which centers the values around the origin.
	// This gives a similar result as subtracting the average X and Y coordinates from the sample. 
	_highpass.x = highPassFilter(_smooth.x, _previousSmooth.x, _highpass.x, HIGH_PASS_ALPHA_COORDINATE);
	_highpass.y = highPassFilter(_smooth.y, _previousSmooth.y, _highpass.y, HIGH_PASS_ALPHA_COORDINATE);

	// We use two ways to determine if there is flow.
	// We have a circular movement, so we have flow if the signal has a certain minimal distance from the origin
	// and if we have a reasonably even angle movement. We check for them both to mimimize the chance of  wrong results.

	const float relativeDistance = _highpass.distanceFromOrigin();

	// The distance tends to fluctuate quite a bit during flow as high pass filters are intrinsically nervous.
	// Smoothening it averages out these fluctuations.

	_smoothRelativeDistance = lowPassFilter(relativeDistance, _smoothRelativeDistance, LOW_PASS_ALPHA_DISTANCE);

	// Values below the low threshold return 0, above the high threshold 1, and in between a gliding linear scale. 
	const auto distanceScore = score(_smoothRelativeDistance, A_DISTANCE, B_DISTANCE);

	// The angle with the origin delivers a value between -PI and PI.
	const auto newAngle = _highpass.angleWithOrigin();

	// Important for flow calculation is the angle difference, which should be even.
	// If flow is off, angles often return noisy values (large differences).
	// We determine whether we have noise by taking the difference, rounding that,
	// and then applying a low pass filter on that. Values close to 0 mean no noise, i.e. flow.

	const auto corDif = fabsf(round(correctedDifference(_angle, newAngle)));
	_cordifLowPass = lowPassFilter(corDif, _cordifLowPass, LOW_PASS_ALPHA_ANGLE);

	// This is another scoring filter, which scores values below the low threshold as 1, above the high threshold as 0
	// and in between a linear scale.
	const auto angleScore = score(_cordifLowPass, A_CORDIF, B_CORDIF);

	// we have flow if the sum of the scores pass an (empirically determined) threshold
	_flow = distanceScore + angleScore > SCORE_THRESHOLD;

	if (_flow) {
		// Now we need to determine the pulses. We have a circular movement. We divide the angle space
		// into quadrants, and we count a pulse every time we move into the negative quadrant (i.e. we pass 0 downwards).

		constexpr float TOP_THRESHOLD = PI / 2;
		constexpr float BOTTOM_THRESHOLD = -TOP_THRESHOLD;

		enum Zones : uint8_t {
			Top = 0,
			Positive,
			Negative,
			Bottom,
			ZoneCount
		};
		// We don't track the zone signal when it's idle, since this is then mainly noise. It's likely that the last
	    // signal we get before the threshold is passed is before it really gets noisy, so it is a reasonable value to keep.
		const int zone = _angle >= TOP_THRESHOLD ? Top : _angle >= 0 ? Positive : _angle >= BOTTOM_THRESHOLD ? Negative : Bottom;

		// hardcoding modulo since it's implementation dependent for negative numbers
		int stateDifference = zone - _state;
		if (stateDifference < 0) stateDifference += ZoneCount;

		// we need to cater for the situation wherre a quadrant is skipped due to large difference.
		// doesn't occur too often, but enough to have to deal with it,.
		const bool peakCandidate = (stateDifference == 1 && zone == Negative) ||
			(stateDifference == 2 && (zone == Negative || zone == Bottom));

		// We use the candidate mechanism to deal with noise in the signal which might jitter a bit between quadrants.
		// we only count a peak if we found a candidate, and we were looking for one

		_peak = peakCandidate && _findNext;

		// Stop looking if we just found a candidate, and start looking again when we left the NEGATIVE quadrant in the right direction
		// Including TOP just in case the signal is too fast and skips BOTTOM (should not happen too often, if at all)

		_findNext = (_findNext && !peakCandidate) || (stateDifference > 0 && (zone == Bottom || zone == Top));
		_state = zone;
	}
	// prepare values for the next call
	_previousSmooth = _smooth;
	_angle = newAngle;
}

float FlowMeter::highPassFilter(const float measure, const float previous, const float filterValue, const float alpha) {
	return alpha * (filterValue + measure - previous);
}

float FlowMeter::lowPassFilter(const float measure, const float filterValue, const float alpha) {
	return alpha * measure + (1 - alpha) * filterValue;
}

void FlowMeter::markAnomalies() {
	// Publish change if needed
	_exclude = _outlier;
	if (_outlier) {
		_consecutiveOutliers++;
		if (_consecutiveOutliers == MAX_CONSECUTIVE_OUTLIERS) {
			_eventServer->publish(Topic::ResetSensor, LONG_TRUE);
		}
		return;
	}
	_consecutiveOutliers = 0;
}

void FlowMeter::resetAnomalies() {
	_outlier = false;
	_flow = false;
	_exclude = false;
}

void FlowMeter::resetFilters(const FloatCoordinate initialSample) {
	_smooth = initialSample;
	_previousSmooth = initialSample;
	_averageAbsoluteDistance = _smooth.distanceFromOrigin();
	_smoothRelativeDistance = 0.0f;
	_angle = -PI;
	// above the threshold to not start with flow too quickly, but not too high to react quick enough if we start with flow
	_cordifLowPass = 0.3f;
	// should not be both 0 as then atan2 is undefined.
	// x and y same to get the angle to round to 1 so we don't start in a flow
	_highpass = { HIGH_PASS_START_VALUE, HIGH_PASS_START_VALUE };
	_findNext = true;
}

void FlowMeter::update(const Topic topic, const long payload) {
	if (topic == Topic::SensorWasReset) {
		// If we needed to reset the sensor, also reset the measurement process when the next sample comes in
		_firstCall = true;
		_consecutiveOutliers = 0;
	}
}

void FlowMeter::update(const Topic topic, const Coordinate payload) {
	if (topic == Topic::Sample) {
		addSample(payload);
	}
}
