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

#include <cstring>
#include <climits>
#include "ResultAggregator.h"

namespace WaterMeter {
    ResultAggregator::ResultAggregator(EventServer* eventServer, Clock* theClock, DataQueue* dataQueue, DataQueuePayload* payload,
        const uint32_t measureIntervalMicros) :
        Aggregator(eventServer, theClock, dataQueue, payload),
        _result(&payload->buffer.result),
        _measureIntervalMicros(measureIntervalMicros) {
        _desiredFlushRate = _idleFlushRate;
    }

    void ResultAggregator::addDuration(const unsigned long duration) const {
        // should only be called once during a message
        _result->totalDuration += duration;
        if (duration > _result->maxDuration) {
            _result->maxDuration = duration;
        }
        // this could be optimized by only getting it executed at the end of a cycle
        _result->averageDuration = static_cast<uint32_t>((_result->totalDuration * 10 / _messageCount + 5) / 10);
    }

    void ResultAggregator::addMeasurement(const SensorSample& value, const FlowDetector* result) {
        newMessage();
        _result->sampleCount = _messageCount;

        if (value.isSaturated() || _result->lastSample == value) {
            _streak++;
            if (_streak > _result->maxStreak) {
                // TODO: fix streak type
                _result->maxStreak = static_cast<uint16_t>(_streak);
            }
        }
        else {
            _streak = 1;
            _result->lastSample = value;
        }
        if (result->foundAnomaly()) {
            _result->anomalyCount++;
        }
        if (result->foundPulse()) {
            _result->pulseCount++;
        }

        if (result->wasReset()) {
            _result->resetCount++;
        }

        if (result->wasSkipped()) {
            _result->skipCount++;
        }

        // we only need this at the end, but we don't know when that is
        _result->ellipseCenterTimes10 = result->ellipseCenterTimes10();
        _result->ellipseRadiusTimes10 = result->ellipseRadiusTimes10();
        _result->ellipseAngleTimes10 = result->ellipseAngleTimes10();
    }

    void ResultAggregator::begin() {
        _eventServer->subscribe(this, Topic::IdleRate);
        _eventServer->subscribe(this, Topic::NonIdleRate);
        _eventServer->subscribe(this, Topic::ProcessTime);
        Aggregator::begin(FlushRateIdle);
    }

    void ResultAggregator::flush() {
        Aggregator::flush();
        _payload->topic = Topic::Result;
        _result->maxStreak = 0;
        _streak = 0;
    }

    bool ResultAggregator::send() {
        const auto wasSuccessful = Aggregator::send();
        if (wasSuccessful) {
            _eventServer->publish(this, Topic::ResultWritten, true);
        }
        return wasSuccessful;
    }

    void ResultAggregator::setIdleFlushRate(const long rate) {
        if (rate != _idleFlushRate) {
            _idleFlushRate = rate;
        }
        setDesiredFlushRate(rate);
    }

    void ResultAggregator::setNonIdleFlushRate(const long rate) {
        if (rate != _nonIdleFlushRate) {
            _nonIdleFlushRate = rate;
        }
    }

    bool ResultAggregator::shouldSend(const bool endOfFile) {
        // We set the flush rate regardless of whether we still need to write something. This can end an idle batch early.
        const bool isInteresting = _result->pulseCount > 0 || _result->anomalyCount > 0;
        if (isInteresting) {
            _flushRate = _nonIdleFlushRate;
        }
        return Aggregator::shouldSend(endOfFile);
    }

    void ResultAggregator::update(const Topic topic, const char* payload) {
        // we only listen to topics with numerical values, so conversion should work
        switch (topic) {
        case Topic::IdleRate:
            update(topic, convertToLong(payload, FlushRateIdle));
            return;
        case Topic::NonIdleRate:
            update(topic, convertToLong(payload, FlushRateInteresting));
            break;
        default:
            break;
        }
    }

    void ResultAggregator::update(const Topic topic, const long payload) {
        const long rate = limit(payload, 0L, LONG_MAX);
        switch (topic) {
        case Topic::IdleRate:
            setIdleFlushRate(rate);
            return;
        case Topic::NonIdleRate:
            setNonIdleFlushRate(rate);
            return;
        case Topic::ProcessTime:
            // safe conversion: process time for once cycle will not be > LONG_MAX (2147 seconds)
            addDuration(payload);
            break;
        default:
            break;
        }
    }
}