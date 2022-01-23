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

void Sampler::setup() {
    _sensorReader->begin();
    _flowMeter->begin();
    _eventServer->subscribe(_queueClient, Topic::Blocked);
    _eventServer->subscribe(_queueClient, Topic::Exclude);
    _eventServer->subscribe(_queueClient, Topic::Flatline);
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
    _nextMeasureTime += MEASURE_INTERVAL_MICROS;
    _eventServer->publish(Topic::Processing, LONG_TRUE);
    const SensorReading measure = _sensorReader->read();

    // this triggers flowMeter and measurementWriter as well as the comms thread
    _eventServer->publish(Topic::Sample, measure.y);
    _resultAggregator->addMeasurement(measure.y, _flowMeter);
    _sampleAggregator->send();
    // Duration gets picked up by resultWriter, so must be published before sending
    const unsigned long measureDurationTimestamp = micros();
    // making sure to use durations to operate on, not timestamps -- to avoid overflow issues
    const unsigned long durationSoFar = measureDurationTimestamp - startLoopTimestamp;
    _eventServer->publish(Topic::ProcessTime, static_cast<long>(durationSoFar  + _additionalDuration));
    _resultAggregator->send();

    _eventServer->publish(Topic::Processing, LONG_FALSE);

    const unsigned long duration = micros() - startLoopTimestamp;
    // adding the missed duration to the next sample. Not entirely accurate, but better than leaving it out
    _additionalDuration = duration - durationSoFar;

    // not using delayMicroseconds() as that is less accurate. Sometimes up to 300 us too much wait time.
    if (duration > MEASURE_INTERVAL_MICROS) {
        // It took too long. If we're still within one interval, we might be able to catch up
        // Intervene if it gets more than that

        _eventServer->publish(Topic::TimeOverrun, static_cast<long>(duration - MEASURE_INTERVAL_MICROS));
        const unsigned long extraTimeNeeded = micros() - _nextMeasureTime;
        _nextMeasureTime += (extraTimeNeeded / MEASURE_INTERVAL_MICROS) * MEASURE_INTERVAL_MICROS;
    }
    else {

        // fill remaining time with validating the connection, checking health and handling incoming messages,
        // but only if we have a minimum amount of time left. We don't want overruns because of this.

        // We use a trick to avoid micros() overrun issues. Create a duration using unsigned long comparison, and then cast to a
        // signed value. We will not pass LONG_MAX in the duration; ESP32 has 64 bit longs and micros() only uses 32 of those.

        auto remainingTime = static_cast<long>(_nextMeasureTime - micros());
        if (remainingTime >= MIN_MICROS_FOR_CHECKS) {
            _device->reportHealth();

            do {
                _queueClient->receive();
                remainingTime = static_cast<long>(_nextMeasureTime - micros());
            } while (remainingTime > 0L);
        }
    }
}
