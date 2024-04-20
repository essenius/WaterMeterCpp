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

// ReSharper disable CppRedundantParentheses - parentheses show intent

#include <cstring>
#include <cstdlib>
#include "Aggregator.h"
#include "DataQueue.h"

namespace WaterMeter {

    Aggregator::Aggregator(EventServer* eventServer, Clock* theClock, DataQueue* dataQueue, DataQueuePayload* payload) :
        EventClient(eventServer),
        _clock(theClock),
        _dataQueue(dataQueue), _payload(payload),
        _flushRate(eventServer, Topic::Rate),
        _blocked(eventServer, Topic::Blocked) {}

    void Aggregator::begin(const long desiredFlushRate) {
        setDesiredFlushRate(desiredFlushRate);
        flush();
    }

    bool Aggregator::canSend() const {
        return _dataQueue->canSend(_payload);
    }

    long Aggregator::convertToLong(const char* stringParam, const long defaultValue) {
        if (strcmp(stringParam, "DEFAULT") == 0) {
            return defaultValue;
        }
        return strtol(stringParam, nullptr, 10);
    }

    void Aggregator::flush() {
        _messageCount = 0;
        for (SensorSample& i : _payload->buffer.samples.value) {
            i.l = 0;
        }
        _flushRate = _desiredFlushRate;
    }

    // Only used for testing, but inherited so can't easily move to a driver
    long Aggregator::getFlushRate() {
        return _flushRate;
    }

    DataQueuePayload* Aggregator::getPayload() const {
        return _payload;
    }

    long Aggregator::limit(const long input, const long min, const long max) {
        return (input < min) ? min : (input > max) ? max : input;
    }

    bool Aggregator::newMessage() {
        if (_flushRate == 0) return false;
        _messageCount++;
        return true;
    }

    bool Aggregator::send() {
        if (!shouldSend()) return false;
        _blocked = !canSend();
        if (_blocked) return false;
        _payload->timestamp = Clock::getTimestamp();
        if (!_dataQueue->send(getPayload())) {
            return false;
        }
        flush();
        return true;
    }

    void Aggregator::setDesiredFlushRate(const long flushRate) {
        _desiredFlushRate = flushRate;
        // Immediately apply if we are starting a new round, or aren't logging
        // Otherwise it will be done at the next reset.
        if (_messageCount == 0 || _flushRate == 0) {
            _flushRate = _desiredFlushRate;
        }
    }

    bool Aggregator::shouldSend(const bool force) {
        if (_flushRate == 0L || _messageCount == 0) {
            return false;
        }
        if (_messageCount % _flushRate != 0 && !force) {
            return false;
        }
        return true;
    }
}