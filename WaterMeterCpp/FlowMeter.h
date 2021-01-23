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
	float getAmplitude();
	float getLowPassFast();
	float getLowPassSlow();
	float getLowPassOnHighPass();
	bool hasDrift();
	bool hasFlow();
	int getPeak();
	bool isExcluded();
	bool isOutlier();
protected:
	static const int STARTUP_SAMPLES = 10;
	float _amplitude = 0.0f;
	bool _calculatedFlow = false;
	int _calculatedPeak = 0;
	float _derivativeAmplitude = 0.0f;
	bool _drift = false;
	bool _exclude = false;
	bool _excludeAll = false;
	bool _firstOutlier = false;
	bool _flow = false;
	float _highPass = 0.0f;
	float _lastValueBeforePeak = 0.0f;
	float _lowPassAmplitude = 0.0f;
	float _lowPassOnDerivativeAmplitude = 0.0f;
	float _lowPassDifference = 0.0f;
	float _lowPassFast = 0.0f;;
	float _lowPassOnDifference = 0.0f;
	float _lowPassOnHighPass = 0.0f;
	float _lowPassSlow = 0.0f;
	bool _outlier = false;
	int _peak = 0;
	int _peakCountDown = 0;
	int _previousMeasure = 0;
	unsigned int _startupSamplesLeft = STARTUP_SAMPLES;

	void detectDrift(int measurement);
	void detectFlow(int measurement);
	void detectOutlier(int measurement);
	void detectPeaks();
	float lowPassFilter(float measure, float filterValue, float alpha);
	float highPassFilter(float measure, float previous, float filterValue, float alpha);
	void markAnomalies(int measurement);
	void resetAnomalies();
	void resetFilters(int initialMeasurement);
	float sign(float value);
};

#endif
