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

#include "Sampler.h"

constexpr int MAX_CONSECUTIVE_OVERRUNS = 3;

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
    // this triggers flowMeter, sampleAggregator and the comms task
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
    if (duration > _samplePeriod) {
        _consecutiveOverrunCount++;
        // It took too long, see if we need to skip measurements to catch up. 
        duration = micros() - _scheduledStartTime;
        // integer mathematics, i.e. no fractions.
        // If we have too many consecutive overruns, skip an extra period.
        const auto shiftPeriod = 
            // ReSharper disable once CppRedundantParentheses - intent clearer this way
            (duration / _samplePeriod) * _samplePeriod + 
            (_consecutiveOverrunCount > MAX_CONSECUTIVE_OVERRUNS ? _samplePeriod : 0);
        _eventServer->publish(Topic::SkipSamples, shiftPeriod /_samplePeriod);
        _scheduledStartTime += shiftPeriod;
        // immediately start the next loop in an attempt to catch up.
    }
    else {
        _consecutiveOverrunCount = 0;
        // Wait for the next sample time; read the command queue while we're at it.
        if (duration < _maxDurationForChecks) {
          _queueClient->receive();
        }

        // now we have the next scheduled start time, so that is in the future.
        duration = micros() - _scheduledStartTime;
        const long delayTime = static_cast<long>(_samplePeriod - duration);

        // delayMicroseconds() is less accurate: sometimes up to 1000 us too much wait time.
        
        if (delayTime > 1000) {
            delayMicroseconds(delayTime - 1000); 
        }
        do {
            duration = micros() - _scheduledStartTime;
        } while (duration < _samplePeriod);
        _scheduledStartTime += _samplePeriod;
    }
}

// if it returns false, the setup failed. Don't try any other functions if so.
bool Sampler::setup(MagnetoSensor* sensor[], const size_t listSize, const unsigned long samplePeriod) {
    _samplePeriod = samplePeriod;
    _maxDurationForChecks = _samplePeriod - _samplePeriod / 5;

    // what can be sent to the communicator (note: must be numerical payloads)
    _eventServer->subscribe(_queueClient, Topic::BatchSize);
    _eventServer->subscribe(_queueClient, Topic::Blocked);
    _eventServer->subscribe(_queueClient, Topic::Exclude);
    _eventServer->subscribe(_queueClient, Topic::Flow);
    _eventServer->subscribe(_queueClient, Topic::FreeQueueSize);
    _eventServer->subscribe(_queueClient, Topic::FreeQueueSpaces);
    _eventServer->subscribe(_queueClient, Topic::Peak);
    _eventServer->subscribe(_queueClient, Topic::ResultWritten);
    _eventServer->subscribe(_queueClient, Topic::Sample);
    _eventServer->subscribe(_queueClient, Topic::SkipSamples);
    _eventServer->subscribe(_queueClient, Topic::TimeOverrun);
    _eventServer->subscribe(_queueClient, Topic::SensorWasReset);
    // SensorReader.begin can publish these     
    _eventServer->subscribe(_queueClient, Topic::Alert);
    _eventServer->subscribe(_queueClient, Topic::NoSensorFound);
    
    if (!_sensorReader->begin(sensor, listSize)) {
        return false;
    }

    _flowMeter->begin(_sensorReader->getNoiseRange(), _sensorReader->getGain());

    return true;
}
