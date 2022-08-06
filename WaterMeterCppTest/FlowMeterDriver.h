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

#pragma once

#include "../WaterMeterCpp/FlowMeter.h"

namespace WaterMeterCppTest {

    class FlowMeterDriver final : public FlowMeter {
    public:
        using FlowMeter::detectPulse;
        using FlowMeter::getTarget;
        using FlowMeter::getSearcher;
        using FlowMeter::_movingAverage;
        using FlowMeter::_smooth;
        using FlowMeter::_flowThresholdPassedCount;
        using FlowMeter::_averageStartValue;
        using FlowMeter::_flowStarted;
        using FlowMeter::_searchTarget;
        using FlowMeter::_currentSearcher;
        using FlowMeter::_averageAbsoluteDistance;

        explicit FlowMeterDriver(EventServer* eventServer) : FlowMeter(eventServer) {}

        /*FlowMeterDriver(EventServer* eventServer, FloatCoordinate smoothValue, FloatCoordinate highPassValue = {0, 0}, float averageAbsoluteDistance = 0,
                        bool flow = false, bool peak = false, bool outlier = false); */


        FlowMeterDriver(EventServer* eventServer, FloatCoordinate smoothValue, bool pulse = false, bool outlier = false);
    };
}
