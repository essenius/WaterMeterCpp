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

class FlowMeter : public EventClient {
public:
    explicit FlowMeter(EventServer* eventServer);
    void begin(int noiseRange, float gain);
    void addSample(int measurement);
    bool areAllExcluded() const;
    float getAmplitude() const;
    float getCombinedDerivative() const;
    float getFastDerivative() const;
    float getFastSmoothValue() const;
    float getSlowSmoothValue() const;
    float getSmoothAbsCombinedDerivative() const;
    float getSmoothAbsFastDerivative() const;
    float getSmoothFastDerivative() const;
    float getZeroThreshold() const;
    bool hasFlow() const;
    bool isExcluded() const;
    bool isOutlier() const;
    bool isPeak() const;
    bool hasEnteredBand() const;
    void update(Topic topic, long payload) override;
    bool wasReset() const;
    static constexpr int SAMPLE_PERIOD_MICROS = 10000;
    static constexpr float SAMPLE_PERIOD_SECONDS = SAMPLE_PERIOD_MICROS / 1000000.0f;

protected:
    static constexpr int MIN_DERIVATIVE_PEAK = -9;
    static constexpr int STARTUP_SAMPLES = 10;
    ChangePublisher<bool> _exclude;
    ChangePublisher<bool> _flow;
    ChangePublisher<bool> _peak;
    float _fastSmooth = 0.0f;
    float _previousFastSmooth = 0.0f;
    float _fastDerivative = 0.0f;
    bool _excludeAll = false;
    bool _firstCall = true;
    bool _outlier = false;
    float _smoothAbsFastDerivative = 0.0f;
    float _smoothFastDerivative = 0.0f;

    unsigned int _startupSamplesLeft = STARTUP_SAMPLES;
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

    void detectOutlier(int measurement);
    void detectPeaks(int measurement);
    static float highPassFilter(float measure, float previous, float filterValue, float alpha);
    static float lowPassFilter(float measure, float filterValue, float alpha);
    void markAnomalies(int measurement);
    void resetAnomalies();
    void resetFilters(int initialMeasurement);
};

#endif
