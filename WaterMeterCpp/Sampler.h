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
    void setup();
    void begin();
    void loop();
    // TODO: eliminate duplication of constants
    static constexpr unsigned long MEASURE_INTERVAL_MICROS = 10UL * 1000UL;
    static constexpr signed long MIN_MICROS_FOR_CHECKS = MEASURE_INTERVAL_MICROS / 5L;

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
};

#endif
