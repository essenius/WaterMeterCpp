// Copyright 2022 Rik Essenius
// 
//   Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//   except in compliance with the License. You may obtain a copy of the License at
// 
//       http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software distributed under the License
//    is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and limitations under the License.

#ifndef HEADER_SAMPLER_H
#define HEADER_SAMPLER_H

#ifdef ESP32
#include <ESP.h>
#else
#include "ArduinoMock.h"
#endif

#include "Device.h"
#include "EventClient.h"
#include "FlowMeter.h"
#include "MagnetoSensorReader.h"
#include "QueueClient.h"
#include "ResultAggregator.h"
#include "SampleAggregator.h"

class Sampler {
public:
    Sampler(EventServer* eventServer, MagnetoSensorReader* sensorReader, FlowMeter* flowMeter,
        SampleAggregator* sampleAggegator, ResultAggregator* resultAggregator, Device* device,  QueueClient* queueClient);
    void setup(unsigned long samplePeriod = 10000UL);
    void begin();
    void loop();

private:
    EventServer* _eventServer;
    MagnetoSensorReader* _sensorReader;
    FlowMeter* _flowMeter;
    SampleAggregator* _sampleAggregator;
    ResultAggregator* _resultAggregator;
    Device* _device;
    QueueClient* _queueClient;
    unsigned long _nextMeasureTime = 0;
    unsigned long _additionalDuration = 0;
    unsigned long _samplePeriod = 10000;
    unsigned long _maxDurationForChecks = 8000;
};

#endif
