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

#include <string.h>
#include <stdlib.h>
#include "BatchWriter.h"

BatchWriter::BatchWriter(const char* name, EventServer* eventServer, PayloadBuilder* payloadBuilder) :
    EventClient(name, eventServer), _payloadBuilder(payloadBuilder), _flushRatePublisher(eventServer, this, Topic::Rate) {
}

void BatchWriter::begin(long desiredFlushRate) {
    setDesiredFlushRate(desiredFlushRate);
    flush();
    resetCounters();
}

long BatchWriter::convertToLong(const char* stringParam, long defaultValue) {
    if (strcmp(stringParam, "DEFAULT") == 0) {
        return defaultValue;
    }
    return strtol(stringParam, 0, 10);
}

void BatchWriter::flush() {
    initBuffer();
    _flushWaiting = false;
}

long BatchWriter::getFlushRate() {
    return _flushRatePublisher.value();
}

const char* BatchWriter::getMessage() {
    return _payloadBuilder->toString();
}

void BatchWriter::initBuffer() {
    _payloadBuilder->initialize();
    _payloadBuilder->writeParam("timestamp", _eventServer->request(Topic::Time, ""));
}

long BatchWriter::limit(long input, long min, long max) {
    return (input < min) ? min : (input > max) ? max : input;
}

// We flush the log every nth time, n being the flush rate. This puts less strain on the receiving end.
// Returns whether a write action took place, so we can ensure writes are in different loops.
bool BatchWriter::needsFlush(bool endOfFile) {
    if (_flushWaiting) {
        return true;
    }
    if (_flushRatePublisher.value() == 0 || _messageCount == 0 || !_canFlush) {
        return false;
    }
    if (_messageCount % _flushRatePublisher.value() != 0 && !endOfFile && !_payloadBuilder->isAlmostFull()) {
        return false;
    }
    prepareFlush();
    return true;
}

bool BatchWriter::newMessage() {
    if (_flushRatePublisher.value() == 0) return false;
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
    _flushRatePublisher.update(_desiredFlushRate);
}

void BatchWriter::setDesiredFlushRate(long flushRate) {
    _desiredFlushRate = flushRate;
    // Immediately apply if we are starting a new round, or aren't logging
    // Otherwise it will be done at the next reset.
    if (_messageCount == 0 || _flushRatePublisher.value() == 0) {
        _flushRatePublisher.update(_desiredFlushRate);
    }
}

/*void BatchWriter::setFlushRate(long flushRate) {
    _flushRatePublisher.update(flushRate);
    if (_flushRate == flushRate) {
        return;
    }
    _flushRate = flushRate;
    _eventServer->publish(_flushRateTopic, flushRate); 
} */

void BatchWriter::update(Topic topic, long payload) {
}
