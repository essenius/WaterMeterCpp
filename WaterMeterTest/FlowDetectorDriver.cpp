// Copyright 2021-2024 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.


#include "FlowDetectorDriver.h"

// constructor for ResultAggregatorTest. Uses fields that are used for reporting

namespace WaterMeterCppTest {
    using WaterMeter::EventServer;
    using EllipseMath::EllipseFit;
    using EllipseMath::Coordinate;

    FlowDetectorDriver::FlowDetectorDriver(EventServer* eventServer, EllipseFit* ellipseFit, const Coordinate average, 
        const bool pulse, const bool outlier, const bool first)
    : FlowDetector(eventServer, ellipseFit) {
        _movingAverage = average;
        _foundPulse = pulse;
        _foundAnomaly = outlier;
        _firstCall = first;
        _wasReset = first;
    }
}
