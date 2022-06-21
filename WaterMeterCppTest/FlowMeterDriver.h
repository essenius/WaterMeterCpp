// Copyright 2021 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#pragma once

#include "../WaterMeterCpp/FlowMeter.h"

class FlowMeterDriver final : public FlowMeter {
public:
    explicit FlowMeterDriver(EventServer* eventServer) : FlowMeter(eventServer) {}

    FlowMeterDriver(EventServer* eventServer, int smoothValue, int derivative = 0, int smoothDerivative = 0,
                             bool flow = false, bool peak = false,
                             bool outlier = false, bool exclude = false);

    float getAverageAbsoluteDistance() const {
	    return _averageAbsoluteDistance;
    }
};

