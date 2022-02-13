// Copyright 2021-2022 Rik Essenius
// 
//   Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//   except in compliance with the License. You may obtain a copy of the License at
// 
//       http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software distributed under the License
//    is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and limitations under the License.

#include "Sampler.h"

Sampler::Sampler(EventServer* eventServer, MagnetoSensorReader* sensorReader, FlowMeter* flowMeter,
                 SampleAggregator* sampleAggegator, ResultAggregator* resultAggregator, QueueClient* queueClient) :
    _eventServer(eventServer), _sensorReader(sensorReader), _flowMeter(flowMeter),
    _sampleAggregator(sampleAggegator), _resultAggregator(resultAggregator), _queueClient(queueClient) {}

void Sampler::begin() {
    // These two publish, so we need to run them when both threads finished setting up the event listeners
    _sampleAggregator->begin();
    _resultAggregator->begin();
    _scheduledStartTime = micros();
}

void Sampler::loop() {
    const int16_t measure = _sensorReader->read();
    // this triggers flowMeter and measurementWriter as well as the comms thread
    _eventServer->publish(Topic::Sample, measure);
    _resultAggregator->addMeasurement(measure, _flowMeter);
    _sampleAggregator->send();
    // Duration gets picked up by resultAggregator, so must be published before sending
    // making sure to use durations to operate on, not timestamps -- to avoid overflow issues
    const unsigned long durationSoFar = micros() - _scheduledStartTime;
    // adding the missed duration to the next sample. Not entirely accurate, but better than leaving it out
    _eventServer->publish(Topic::ProcessTime, static_cast<long>(durationSoFar  + _additionalDuration));
    _resultAggregator->send();

    unsigned long duration = micros() - _scheduledStartTime;
    _additionalDuration = duration - durationSoFar;

    // not using delayMicroseconds() as that is less accurate. Sometimes up to 300 us too much wait time.
    // This is the only task on this core, so no need to yield either.
    if (duration > _samplePeriod) {
        // It took too long. If we're still within one interval, we might be able to catch up
        // Intervene if it gets more than that
        _scheduledStartTime += (duration / _samplePeriod) * _samplePeriod;
    }
    else {
        // Wait for the next sample time; read the command queue while we're at it.
        // No need to use delay since we
        do {
            _queueClient->receive();
            duration = micros() - _scheduledStartTime;
        } while (duration < _samplePeriod);
        _scheduledStartTime += _samplePeriod;
    }
}

void Sampler::setup(const unsigned long samplePeriod) {
    _samplePeriod = samplePeriod;
    _maxDurationForChecks = _samplePeriod - _samplePeriod / 5;
    _sensorReader->begin();
    _flowMeter->begin();
    // what can be sent to the communicator (note: must be numerical payload)
    _eventServer->subscribe(_queueClient, Topic::Alert);
    _eventServer->subscribe(_queueClient, Topic::BatchSize);
    _eventServer->subscribe(_queueClient, Topic::Blocked);
    _eventServer->subscribe(_queueClient, Topic::Exclude);
    _eventServer->subscribe(_queueClient, Topic::Flow);
    _eventServer->subscribe(_queueClient, Topic::FreeQueueSize);
    _eventServer->subscribe(_queueClient, Topic::FreeQueueSpaces);
    _eventServer->subscribe(_queueClient, Topic::Peak);
    _eventServer->subscribe(_queueClient, Topic::ResultWritten);
    _eventServer->subscribe(_queueClient, Topic::Sample);
    _eventServer->subscribe(_queueClient, Topic::TimeOverrun);
    _eventServer->subscribe(_queueClient, Topic::SensorWasReset);
}
