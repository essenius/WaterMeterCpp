// Copyright 2021 Rik Essenius
// 
//   Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//   except in compliance with the License. You may obtain a copy of the License at
// 
//       http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software distributed under the License
//    is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and limitations under the License.

#include "MeasurementWriter.h"
#include <string.h>

MeasurementWriter::MeasurementWriter(EventServer* eventServer, PayloadBuilder* payloadBuilder) :
    BatchWriter("MeasurementWriter", eventServer, payloadBuilder) {
    _flushRatePublisher.setTopic(Topic::BatchSize);
}

void MeasurementWriter::addMeasurement(int measure) {
    // Only record measurements if we need to
    if (newMessage() && _canFlush) {
        _payloadBuilder->writeArrayValue(measure);
    }
    else {
        if (_messageCount > 0) {
            // If we can't flush and we have messages already, discard them
            flush();
            resetCounters();
        }
    }
}

void MeasurementWriter::begin() {
    _eventServer->subscribe(this, Topic::BatchSizeDesired);
    BatchWriter::begin(DEFAULT_FLUSH_RATE);
    // This is the only time the desired rate gets published from here.
    _eventServer->publish(this, Topic::BatchSizeDesired, _desiredFlushRate);
}

void MeasurementWriter::initBuffer() {
    BatchWriter::initBuffer();
    _payloadBuilder->writeArrayStart("measurements");
}

void MeasurementWriter::prepareFlush() {
    _payloadBuilder->writeArrayEnd();
    BatchWriter::prepareFlush();
}

void MeasurementWriter::update(Topic topic, const char* payload) {
    if (topic == Topic::BatchSizeDesired) {
        auto desiredRate = convertToLong(payload, DEFAULT_FLUSH_RATE);
        return update(topic, desiredRate);
    } 
    BatchWriter::update(topic, payload);
}

void MeasurementWriter::update(Topic topic, long payload) {
    if (topic == Topic::BatchSizeDesired) {
        unsigned char desiredRate = (unsigned char)limit(payload, 0L, MAX_FLUSH_RATE);
        setDesiredFlushRate(desiredRate);
        return;
    }
    BatchWriter::update(topic, payload);
}
