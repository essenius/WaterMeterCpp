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

#include <cstring>
#include <climits>
#include "ResultAggregator.h"

ResultAggregator::ResultAggregator(EventServer* eventServer, Clock* theClock, DataQueue* dataQueue, SensorDataQueuePayload* payload,
                                   const uint32_t measureIntervalMicros) :
    Aggregator(eventServer, theClock, dataQueue, payload),
    _result(&payload->buffer.result),
    _overrun(eventServer, this, Topic::TimeOverrun, 0L),
    _measureIntervalMicros(measureIntervalMicros) {
    _desiredFlushRate = _idleFlushRate;
}

void ResultAggregator::addDuration(const uint32_t duration) {
    // should only be called once during a message
    _result->totalDuration += duration;
    if (duration > _result->maxDuration) {
        _result->maxDuration = duration;
    }
    // this works because duration (and measure interval) will not be larger than 2,147 seconds, so will not overflow to negative. 
    _overrun = std::max(0L, static_cast<long>(duration - _measureIntervalMicros));
    if (_overrun > 0) {
        _result->overrunCount++;
    }
    // this could be optimized by only getting it executed at the end of a cycle
    _result->averageDuration = static_cast<uint32_t>((_result->totalDuration * 10 / _messageCount + 5) / 10);

}

void ResultAggregator::addMeasurement(const int16_t value, const FlowMeter* result) {
    newMessage();
    _result->sampleCount = _messageCount;

    if (_result->lastSample == value) {
        _streak++;
        if (_streak > _result->maxStreak) {
            _result->maxStreak = _streak;
        }
    }
    else {
        _streak = 1;
        _result->lastSample = value;
    }
    if (result->isOutlier()) {
        _result->outlierCount++;
    }
    if (result->isPeak()) {
        _result->peakCount++;
    }
    if (result->hasFlow()) {
        _result->flowCount++;
    }

    if (result->areAllExcluded()) {
        _result->excludeCount = _messageCount;
    }
    else if (result->isExcluded()) {
        _result->excludeCount++;
    }
    // we only need these at the end but we don't know when that is
    _result->smooth = result->getSmoothValue();
    _result->derivativeSmooth = result->getDerivative();
    _result->smoothDerivativeSmooth = result->getSmoothDerivative();
    _result->smoothAbsDerivativeSmooth = result->getSmoothAbsDerivative();
}

void ResultAggregator::begin() {
    _eventServer->subscribe(this, Topic::IdleRate);
    _eventServer->subscribe(this, Topic::NonIdleRate);
    _eventServer->subscribe(this, Topic::ProcessTime);
    Aggregator::begin(FLUSH_RATE_IDLE);
}

void ResultAggregator::flush() {
    Aggregator::flush();
    _payload->topic = Topic::Result;
    _result->maxStreak = 1;
    _streak = 1;
}

bool ResultAggregator::send() {
    const auto wasSuccessful = Aggregator::send();
    if (wasSuccessful) {
        _eventServer->publish(this, Topic::ResultWritten, LONG_TRUE);
        _eventServer->publish(this, Topic::TimeOverrun, LONG_FALSE);
    }
    return wasSuccessful;
}

void ResultAggregator::setIdleFlushRate(long rate) {
    if (rate != _idleFlushRate) {
        _idleFlushRate = rate;
    }
    setDesiredFlushRate(rate);
}

void ResultAggregator::setNonIdleFlushRate(long rate) {
    if (rate != _nonIdleFlushRate) {
        _nonIdleFlushRate = rate;
    }
}

bool ResultAggregator::shouldSend(const bool endOfFile) {
    // We set the flush rate regardless of whether we still need to write something. This can end an idle batch early.
    const bool isInteresting = _result->flowCount > 0 || _result->excludeCount > 0;
    if (isInteresting) {
        _flushRate = _nonIdleFlushRate;
    }
    return Aggregator::shouldSend(endOfFile);
}

void ResultAggregator::update(Topic topic, const char* payload) {
    // we only listen to topics with numerical values, so conversion should work
    switch (topic) {
    case Topic::IdleRate:
        update(topic, convertToLong(payload, FLUSH_RATE_IDLE));
        return;
    case Topic::NonIdleRate:
        update(topic, convertToLong(payload, FLUSH_RATE_INTERESTING));
        break;
    default:
        break;
    }
}

void ResultAggregator::update(Topic topic, long payload) {
    const long rate = limit(payload, 0L, LONG_MAX);
    switch (topic) {
    case Topic::IdleRate:
        setIdleFlushRate(rate);
        return;
    case Topic::NonIdleRate:
        setNonIdleFlushRate(rate);
        return;
    case Topic::ProcessTime:
        addDuration(payload);
        break;
    default:
        break;
    }
}
