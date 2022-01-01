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
#include <cstdlib>
#include "BatchWriter.h"

BatchWriter::BatchWriter(const char* name, EventServer* eventServer, PayloadBuilder* payloadBuilder) :
    EventClient(name, eventServer), _payloadBuilder(payloadBuilder), _flushRatePublisher(eventServer, this, Topic::Rate) {
}

void BatchWriter::begin(long desiredFlushRate) {
    setDesiredFlushRate(desiredFlushRate);
    flush();
    resetCounters();
    _eventServer->subscribe(this, Topic::Connected);
    _eventServer->subscribe(this, Topic::Disconnected);
}

long BatchWriter::convertToLong(const char* stringParam, const long defaultValue) {
    if (strcmp(stringParam, "DEFAULT") == 0) {
        return defaultValue;
    }
    return strtol(stringParam, nullptr, 10);
}

void BatchWriter::flush() {
    initBuffer();
    _flushWaiting = false;
}

long BatchWriter::getFlushRate() {
    return _flushRatePublisher.get();
}

const char* BatchWriter::getMessage() const {
    return _payloadBuilder->toString();
}

void BatchWriter::initBuffer() {
    _payloadBuilder->initialize();
    _payloadBuilder->writeParam("timestamp", _eventServer->request(Topic::Time, ""));
}

long BatchWriter::limit(long input, long min, long max) {
    return (input < min) ? min : (input > max) ? max : input;
}

// Returns whether a write action should take place.
// We flush the log every nth time, n being the flush rate. This puts less strain on the receiving end.
// The scheduler could decide to postpone the write if another writer has priority.
bool BatchWriter::needsFlush(bool force) {
    if (_flushWaiting && _canFlush) {
        return true;
    }
    if (_flushRatePublisher.get() == 0 || _messageCount == 0 || !_canFlush) {
        return false;
    }
    if (_messageCount % _flushRatePublisher.get() != 0 && !force && !_payloadBuilder->isAlmostFull()) {
        return false;
    }
    prepareFlush();
    return true;
}

bool BatchWriter::newMessage() {
    if (_flushRatePublisher.get() == 0) return false;
    _messageCount++;
    return true;
}

void BatchWriter::prepareFlush() {
    // Now we can start recording the next round. The buffer isn't necessarily flushed in the same cycle as it's prepared.
    _payloadBuilder->writeGroupEnd();
    resetCounters();
    _flushWaiting = true;
}

void BatchWriter::resetCounters() {
    _messageCount = 0;
    _flushRatePublisher.set(_desiredFlushRate);
}

void BatchWriter::setDesiredFlushRate(long flushRate) {
    _desiredFlushRate = flushRate;
    // Immediately apply if we are starting a new round, or aren't logging
    // Otherwise it will be done at the next reset.
    if (_messageCount == 0 || _flushRatePublisher.get() == 0) {
        _flushRatePublisher.set(_desiredFlushRate);
    }
}

void BatchWriter::update(Topic topic, const char* payload) {
    switch (topic) {
    case Topic::Connected:
        _canFlush = true;
        break;
    case Topic::Disconnected:
        _canFlush = false;
        break;
    default: {}
           // ignore
    }
}

void BatchWriter::update(Topic topic, long payload) {
    // shortcut, we don't use the payloads for the topics we're interested in
    return BatchWriter::update(topic, "");
}
