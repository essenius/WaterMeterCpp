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

#include "SampleAggregator.h"

SampleAggregator::SampleAggregator(EventServer* eventServer, Clock* theClock, DataQueue* dataQueue, DataQueuePayload* payload) :
    Aggregator(eventServer, theClock, dataQueue, payload) {
    _flushRate.setTopic(Topic::BatchSize);
}

void SampleAggregator::addSample(const int16_t measure) {
    // Only record measurements if we need to
    if (newMessage() && canSend()) {

        _payload->buffer.samples.value[_payload->buffer.samples.count++] = measure;
    }
    else {
        if (_messageCount > 0) {
            // If we can't send and we have messages already, discard them (beats crashing)
            flush();
        }
    }
}

void SampleAggregator::begin() {
    _eventServer->subscribe(this, Topic::BatchSizeDesired);
    _eventServer->subscribe(this, Topic::Sample);
    Aggregator::begin(DEFAULT_FLUSH_RATE);
    // This is the only time the desired rate gets published from here.
    _eventServer->publish(this, Topic::BatchSizeDesired, _desiredFlushRate);
}

void SampleAggregator::flush() {
    Aggregator::flush();
    _payload->topic = Topic::Samples;
    _payload->buffer.samples.count = 0;
}

void SampleAggregator::update(const Topic topic, const char* payload) {
    if (topic == Topic::BatchSizeDesired) {
        const auto desiredRate = convertToLong(payload, DEFAULT_FLUSH_RATE);
        update(topic, desiredRate);
    }
}

void SampleAggregator::update(const Topic topic, const long payload) {
    if (topic == Topic::BatchSizeDesired) {
        const auto desiredRate = static_cast<unsigned char>(limit(payload, 0L, MAX_FLUSH_RATE));
        setDesiredFlushRate(desiredRate);
        return;
    }
    if (topic == Topic::Sample) {
        addSample(static_cast<int16_t>(payload));
    }
}
