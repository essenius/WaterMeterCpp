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

// This runs the process that handles the sampling. It must ensure that each loop takes less than 10ms,
// and that there is regularly enough (>2ms) time available to handle input.

#ifndef HEADER_SAMPLER
#define HEADER_SAMPLER

#include "Button.h"
#include "EventClient.h"
#include "FlowDetector.h"
#include "MagnetoSensorReader.h"
#include "QueueClient.h"
#include "ResultAggregator.h"
#include "SampleAggregator.h"

namespace WaterMeter {
    class Sampler {
    public:
        Sampler(EventServer* eventServer, MagnetoSensorReader* sensorReader, FlowDetector* flowDetector, Button* button,
            SampleAggregator* sampleAggregator, ResultAggregator* resultAggregator, QueueClient* queueClient);
        bool begin(MagnetoSensor* sensor[], size_t listSize = 3, unsigned long samplePeriod = 10000UL);
        void beginLoop(TaskHandle_t taskHandle);
        void loop();
        static void task(void* parameter);

    protected:
        static constexpr byte TimerNumber = 0;
        static constexpr unsigned short Divider = 80; // 80 MHz -> 1 MHz
        static constexpr UBaseType_t SampleQueueSize = 50;
        static constexpr UBaseType_t OverrunQueueSize = 20;
        static constexpr unsigned long MaxOffsetMicros = 100;
        static constexpr bool Repeat = true;
        static constexpr bool CountUp = true;
        static constexpr bool Edge = true;

        EventServer* _eventServer;
        MagnetoSensorReader* _sensorReader;
        FlowDetector* _flowDetector;
        Button* _button;
        SampleAggregator* _sampleAggregator;
        ResultAggregator* _resultAggregator;
        QueueClient* _queueClient;
        unsigned long _additionalDuration = 0;
        unsigned long _samplePeriod = 10000;
        unsigned long _ticksPerSample = 10;
        unsigned long _previousReadTime = 0;
        long _previousOverrun = 0;

        hw_timer_t* _timer = nullptr;
        QueueHandle_t _sampleQueue = nullptr;
        QueueHandle_t _overrunQueue = nullptr;
        static TaskHandle_t _taskHandle;
        static volatile unsigned long _interruptCounter;
        volatile unsigned long _notifyCounter = 0;
        volatile unsigned long _queueFullCounter = 0;
        unsigned long _sampleCount = 0;
        unsigned long _overruns = 0;

        static void ARDUINO_ISR_ATTR onTimer();
        void handleSample(const IntCoordinate& sample, unsigned long startTime);

        void sensorLoop();

    };
}
#endif
