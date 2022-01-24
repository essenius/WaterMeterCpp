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
                 SampleAggregator* sampleAggegator, ResultAggregator* resultAggregator, Device* device, QueueClient* queueClient) :
    _eventServer(eventServer), _sensorReader(sensorReader), _flowMeter(flowMeter),
    _sampleAggregator(sampleAggegator), _resultAggregator(resultAggregator), _device(device), _queueClient(queueClient) {}

void Sampler::setup(const unsigned long samplePeriod) {
    _samplePeriod = samplePeriod;
    _maxDurationForChecks = _samplePeriod - _samplePeriod / 5;
    _sensorReader->begin();
    _flowMeter->begin();
    _eventServer->subscribe(_queueClient, Topic::Blocked);
    _eventServer->subscribe(_queueClient, Topic::Exclude);
    _eventServer->subscribe(_queueClient, Topic::Flow);
    _eventServer->subscribe(_queueClient, Topic::Peak);
    _eventServer->subscribe(_queueClient, Topic::Processing);
    _eventServer->subscribe(_queueClient, Topic::Sample);
    _eventServer->subscribe(_queueClient, Topic::TimeOverrun);
}

void Sampler::begin() {
    // These two publish, so we need to run them when both threads finished setting up the event listeners
    _sampleAggregator->begin(); 
    _resultAggregator->begin();
    _nextMeasureTime = micros();
}

void Sampler::loop() {
    const unsigned long startLoopTimestamp = micros();
    _nextMeasureTime += _samplePeriod;
    _eventServer->publish(Topic::Processing, LONG_TRUE);
    const int16_t measure = _sensorReader->read();

    // this triggers flowMeter and measurementWriter as well as the comms thread
    _eventServer->publish(Topic::Sample, measure);
    _resultAggregator->addMeasurement(measure, _flowMeter);
    _sampleAggregator->send();
    // Duration gets picked up by resultWriter, so must be published before sending
    const unsigned long measureDurationTimestamp = micros();
    // making sure to use durations to operate on, not timestamps -- to avoid overflow issues
    const unsigned long durationSoFar = measureDurationTimestamp - startLoopTimestamp;
    _eventServer->publish(Topic::ProcessTime, static_cast<long>(durationSoFar  + _additionalDuration));
    _resultAggregator->send();

    _eventServer->publish(Topic::Processing, LONG_FALSE);

    unsigned long duration = micros() - startLoopTimestamp;
    // adding the missed duration to the next sample. Not entirely accurate, but better than leaving it out
    _additionalDuration = duration - durationSoFar;

    // not using delayMicroseconds() as that is less accurate. Sometimes up to 300 us too much wait time.
    if (duration > _samplePeriod) {
        // It took too long. If we're still within one interval, we might be able to catch up
        // Intervene if it gets more than that

        _eventServer->publish(Topic::TimeOverrun, static_cast<long>(duration - _samplePeriod));
        const unsigned long extraTimeNeeded = micros() - _nextMeasureTime;
        _nextMeasureTime += (extraTimeNeeded / _samplePeriod) * _samplePeriod;
    }
    else {
        // we only do additional work if we have enough time left.
        if (duration <= _maxDurationForChecks) {
            _device->reportHealth();

            do {
                _queueClient->receive();
                duration = micros() - startLoopTimestamp;
            } while (duration < _samplePeriod);
        }
    }
}
