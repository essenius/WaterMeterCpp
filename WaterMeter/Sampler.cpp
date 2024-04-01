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

// TODO: generation of Alert

#include "Sampler.h"

namespace WaterMeter {

    TaskHandle_t Sampler::_taskHandle = nullptr;
    volatile unsigned long Sampler::_interruptCounter = 0;

    Sampler::Sampler(EventServer* eventServer, MagnetoSensorReader* sensorReader, FlowDetector* flowDetector, Button* button,
        SampleAggregator* sampleAggregator, ResultAggregator* resultAggregator, QueueClient* queueClient) :
        _eventServer(eventServer), _sensorReader(sensorReader), _flowDetector(flowDetector), _button(button),
        _sampleAggregator(sampleAggregator), _resultAggregator(resultAggregator), _queueClient(queueClient) {
        _sampleQueue = xQueueCreate(SampleQueueSize, sizeof(SensorSample));
        _overrunQueue = xQueueCreate(OverrunQueueSize, sizeof(long));
    }

    void ARDUINO_ISR_ATTR Sampler::onTimer() {
        _interruptCounter++;
        vTaskNotifyGiveFromISR(_taskHandle, nullptr);
    }

    // if it returns false, the setup failed. Don't try any other functions if so.
    bool Sampler::begin(MagnetoSensor* sensor[], const size_t listSize, const unsigned long samplePeriod) {
        _samplePeriod = samplePeriod;
        _ticksPerSample = samplePeriod / 1000UL;
        _button->begin();
        // what can be sent to the communicator (note: must be numerical payloads)
        _eventServer->subscribe(_queueClient, Topic::BatchSize);
        _eventServer->subscribe(_queueClient, Topic::Blocked);
        _eventServer->subscribe(_queueClient, Topic::Anomaly);
        _eventServer->subscribe(_queueClient, Topic::FreeQueueSize);
        _eventServer->subscribe(_queueClient, Topic::FreeQueueSpaces);
        _eventServer->subscribe(_queueClient, Topic::NoFit);
        _eventServer->subscribe(_queueClient, Topic::Pulse);
        _eventServer->subscribe(_queueClient, Topic::ResultWritten);
        _eventServer->subscribe(_queueClient, Topic::Sample);
        _eventServer->subscribe(_queueClient, Topic::SensorWasReset);
        _eventServer->subscribe(_queueClient, Topic::SkipSamples);
        _eventServer->subscribe(_queueClient, Topic::TimeOverrun);
        // SensorReader.begin can publish these     
        _eventServer->subscribe(_queueClient, Topic::Alert);
        _eventServer->subscribe(_queueClient, Topic::SensorState);

        if (!_sensorReader->begin(sensor, listSize)) {
            return false;
        }

        _flowDetector->begin(_sensorReader->getNoiseRange());

        return true;
    }

    void Sampler::beginLoop(const TaskHandle_t taskHandle) {
        _taskHandle = taskHandle;
        // These two publish, so we need to run them when both threads finished setting up the event listeners
        _sampleAggregator->begin();
        _resultAggregator->begin();

        // start the timer. The task should already be listening.

        _timer = timerBegin(TimerNumber, Divider, CountUp);
        timerAttachInterrupt(_timer, &Sampler::onTimer, Edge);
        timerAlarmWrite(_timer, _samplePeriod, Repeat);
        timerAlarmEnable(_timer);
    }

    /**
     * \brief Wait for a notification from the timer, get a sensor reading and put it on a queue. Also check for time overrun.
     * This runs in a different thread, so we need queues to communicate.
     */
    void Sampler::sensorLoop() {
        if (ulTaskNotifyTake(pdTRUE, _ticksPerSample) > 0) {
            _notifyCounter++;
            const auto lastReadTime = micros();
            const SensorSample sample = _sensorReader->read();
            if (uxQueueSpacesAvailable(_sampleQueue) == 0) {
                _queueFullCounter++;
                //overrun = _samplePeriod;
                //xQueueSendToBack(_overrunQueue, &overrun, 0);
            }
            else {
                xQueueSendToBack(_sampleQueue, &sample, 0);
                // if we just started, do not calculate overrun
                if (_notifyCounter > 1) {
                    const auto timeSincePreviousSample = lastReadTime - _previousReadTime;
                    const long overrun = timeSincePreviousSample > _samplePeriod + MaxOffsetMicros ? static_cast<long>(timeSincePreviousSample - _samplePeriod) : 0L;
                    if (overrun != _previousOverrun) {
                        xQueueSendToBack(_overrunQueue, &overrun, 0);
                        _previousOverrun = overrun;
                    }
                }
            }
            _previousReadTime = lastReadTime;
        }
    }

    /**
     * \brief The less time sensitive activities that need to be handled in a sample.
     * As long as on average the time stays below the sample time we're good.
     */
    void Sampler::loop() {
        SensorSample sample{};
        const auto startTime = micros();
        long overrun = 0;
        while (xQueueReceive(_overrunQueue, &overrun, 0) == pdTRUE) {
            _overruns++;
            _eventServer->publish(Topic::TimeOverrun, overrun);
        }

        while (xQueueReceive(_sampleQueue, &sample, _ticksPerSample) == pdTRUE) {
            _sampleCount++;
            const auto state = _sensorReader->validate(sample);
            switch (state) {
            case SensorState::NeedsSoftReset:
                _sensorReader->softReset();
                break;
            case SensorState::NeedsHardReset:
                _sensorReader->hardReset();
                break;
            case SensorState::ReadError:
            case SensorState::Saturated:
            case SensorState::Ok:
                handleSample(sample, startTime);
                break;
            default: {}
            }
            //printf(".");
            _queueClient->receive();
            //printf("$");
            _button->check();
        }
    }

    void Sampler::handleSample(const SensorSample& sample, const unsigned long startTime) {
        // this triggers flowDetector, sampleAggregator and the comms task
        //printf("@");
        _eventServer->publish(Topic::Sample, sample);
        _resultAggregator->addMeasurement(sample, _flowDetector);
        _sampleAggregator->send();
        // Duration gets picked up by resultAggregator, so must be published before sending
        // making sure to use durations to operate on, not timestamps -- to avoid overflow issues
        const auto durationSoFar = micros() - startTime;
        // adding the missed duration to the next sample. Not entirely accurate, but better than leaving it out
        _eventServer->publish(Topic::ProcessTime, static_cast<long>(durationSoFar + _additionalDuration));
        _resultAggregator->send();
        const auto duration = micros() - startTime;
        _additionalDuration = duration - durationSoFar;
    }

    [[ noreturn ]] void Sampler::task(void* parameter) {
        const auto me = static_cast<Sampler*>(parameter);
        for (;;) {
            me->sensorLoop();
        }
    }
}