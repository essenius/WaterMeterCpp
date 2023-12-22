#pragma once
#include "../WaterMeterCpp/Sampler.h"

namespace WaterMeterCppTest {
    using namespace WaterMeter;

    class SamplerDriver : public Sampler {
    public:
        using Sampler::onTimer;
        using Sampler::sensorLoop;

        SamplerDriver(EventServer* eventServer, MagnetoSensorReader* sensorReader, FlowDetector* flowDetector, Button* button,
            SampleAggregator* sampleAggegator, ResultAggregator* resultAggregator, QueueClient* queueClient)
            : Sampler(eventServer, sensorReader, flowDetector, button, sampleAggegator, resultAggregator, queueClient) {}


    };
}
