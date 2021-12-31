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

#ifndef HEADER_FLOWMETER
#define HEADER_FLOWMETER

class FlowMeter {
public:
    void addMeasurement(int measurement);
    bool areAllExcluded();
    float getSmoothValue();
    float getDerivative();
    float getSmoothAbsDerivative();
    float getSmoothDerivative();
    bool isPeak();
    bool hasFlow();
    bool isExcluded();
    bool isOutlier();
protected:
    static const int STARTUP_SAMPLES = 10;
    static const int MIN_DERIVATIVE_PEAK = -9;
    bool _exclude = false;
    bool _excludeAll = false;
    bool _firstOutlier = false;
    float _derivative = 0.0f;
    float _smoothValue = 0.0f;
    float _smoothDerivative = 0.0f;
    float _smoothAbsDerivative = 0.0f;
    float _minDerivative = MIN_DERIVATIVE_PEAK;
    float _previousSmoothDerivative = 0.0f;
    float _previousSmoothValue = 0.0f;
    bool _flow = false;
    bool _outlier = false;
    bool _peak = false;
    unsigned int _startupSamplesLeft = STARTUP_SAMPLES;

    void detectOutlier(int measurement);
    void detectPeaks(int measurement);
    float highPassFilter(float measure, float previous, float filterValue, float alpha);
    float lowPassFilter(float measure, float filterValue, float alpha);
    void markAnomalies(int measurement);
    void resetAnomalies();
    void resetFilters(int initialMeasurement);
};

#endif
