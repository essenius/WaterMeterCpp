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

#ifndef HEADER_FLOWMETER
#define HEADER_FLOWMETER
#include "ChangePublisher.h"
#include "EventClient.h"

class FlowMeter: public EventClient {
public:
    explicit FlowMeter(EventServer* eventServer);
    void begin();
    void addSample(int measurement);
    bool areAllExcluded() const;
    float getSmoothValue() const;
    float getDerivative() const;
    float getSmoothAbsDerivative() const;
    float getSmoothDerivative() const;
    bool isPeak() const;
    bool hasFlow() const;
    bool isExcluded() const;
    bool isOutlier() const;
    void update(Topic topic, long payload) override;

protected:
    static constexpr int STARTUP_SAMPLES = 10;
    static constexpr int MIN_DERIVATIVE_PEAK = -9;
    ChangePublisher<bool> _exclude;
    bool _excludeAll = false;
    float _derivative = 0.0f;
    float _smoothValue = 0.0f;
    float _smoothDerivative = 0.0f;
    float _smoothAbsDerivative = 0.0f;
    float _minDerivative = MIN_DERIVATIVE_PEAK;
    float _previousSmoothDerivative = 0.0f;
    float _previousSmoothValue = 0.0f;
    ChangePublisher<bool> _flow;
    ChangePublisher<bool> _peak;
    bool _outlier = false;
    unsigned int _startupSamplesLeft = STARTUP_SAMPLES;

    void detectOutlier(int measurement);
    void detectPeaks(int measurement);
    static float highPassFilter(float measure, float previous, float filterValue, float alpha);
    static float lowPassFilter(float measure, float filterValue, float alpha);
    void markAnomalies(int measurement);
    void resetAnomalies();
    void resetFilters(int initialMeasurement);
};

#endif
