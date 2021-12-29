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

 #include "pch.h"

#include "FlowMeterDriver.h"

// constructor for ResultWriterTest. Uses fields that are used for reporting
FlowMeterDriver::FlowMeterDriver(int smoothValue, int derivative, int smoothDerivative,
	bool flow, bool peak, bool outlier, bool exclude, bool excludeAll) {
	_smoothValue = static_cast<float>(smoothValue);
	_derivative = static_cast<float>(derivative);
	_smoothDerivative = static_cast<float>(smoothDerivative);
	_smoothAbsDerivative = 0.0f;
	_exclude = exclude;
	_excludeAll = excludeAll;
	_flow = flow;
	_outlier = outlier;
	_peak = peak;
}


float FlowMeterDriver::isFirstOutlier() {
	return _firstOutlier;
}
