// Copyright 2022-2024 Rik Essenius
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
#include "../WaterMeter/Sampler.h"

namespace WaterMeterCppTest {
    using namespace WaterMeter;

    class SamplerDriver : public Sampler {
    public:
        using Sampler::onTimer;
        using Sampler::sensorLoop;

        SamplerDriver(EventServer* eventServer, MagnetoSensorReader* sensorReader, FlowDetector* flowDetector, Button* button,
            SampleAggregator* sampleAggregator, ResultAggregator* resultAggregator, QueueClient* queueClient)
            : Sampler(eventServer, sensorReader, flowDetector, button, sampleAggregator, resultAggregator, queueClient) {}


    };
}