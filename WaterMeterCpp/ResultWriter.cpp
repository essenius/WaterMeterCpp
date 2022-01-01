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

#include <string.h>
#include <limits.h>
#include "ResultWriter.h"

ResultWriter::ResultWriter(EventServer* eventServer, PayloadBuilder* payloadBuilder, int measureIntervalMicros) :
    BatchWriter("ResultWriter", eventServer, payloadBuilder) {
    _measureIntervalMicros = measureIntervalMicros;
    _desiredFlushRate = _idleFlushRate;
}

void ResultWriter::addDuration(long duration) {
    // should only be called once during a message
    _sumDuration += duration;
    if (_maxDuration < duration) {
        _maxDuration = duration;
    }
    if (duration > _measureIntervalMicros) {
        publishOverrun(true);
        _overrunCount++;
    }
}

void ResultWriter::addMeasurement(int value, FlowMeter* result) {
    newMessage();
    _measure = value;
    //_result = result;
    if (result->isOutlier()) {
        _outlierCount++;
    }
    if (result->isPeak()) {
        _peakCount++;
    }
    if (result->hasFlow()) {
        _flowCount++;
    }
    if (result->areAllExcluded()) {
        _excludeCount = _messageCount;
    }
    else if (result->isExcluded()) {
        _excludeCount++;
    }
    // we only need these at the end but we don't know when that is
    _smoothValue = result->getSmoothValue();
    _derivative = result->getDerivative();
    _smoothDerivative = result ->getSmoothDerivative();
    _smoothAbsDerivative = result->getSmoothAbsDerivative();
}

void ResultWriter::begin() {
    _eventServer->subscribe(this, Topic::IdleRate);
    _eventServer->subscribe(this, Topic::NonIdleRate);
    _eventServer->subscribe(this, Topic::ProcessTime);
    BatchWriter::begin(FLUSH_RATE_IDLE);

    // This is the only time the desired rates get published from here.
    _eventServer->publish(this, Topic::IdleRate, _idleFlushRate);
    _eventServer->publish(this, Topic::NonIdleRate, _nonIdleFlushRate);
}

bool ResultWriter::needsFlush(bool endOfFile) {
    // We set the flush rate regardless of whether we still need to write something. This can end an idle batch early.
    bool isInteresting = _flowCount > 0 || _excludeCount > 0;
    if (isInteresting) {
        _flushRatePublisher.set(_nonIdleFlushRate);
    }
    return BatchWriter::needsFlush(endOfFile);
}

void ResultWriter::publishOverrun(bool overrun) {
    if (_overrun != overrun) {
        _eventServer->publish<long>(Topic::TimeOverrun, overrun);
        _overrun = overrun;
    }
}

void ResultWriter::prepareFlush() {
    int averageDuration = static_cast<int>((_sumDuration * 10 / _messageCount + 5) / 10);
    _payloadBuilder->writeParam("lastValue", _measure);
    _payloadBuilder->writeGroupStart("summaryCount");
    _payloadBuilder->writeParam("samples", _messageCount);
    _payloadBuilder->writeParam("peaks", _peakCount);
    _payloadBuilder->writeParam("flows", _flowCount);
    _payloadBuilder->writeGroupEnd();
    _payloadBuilder->writeGroupStart("exceptionCount");
    _payloadBuilder->writeParam("outliers", _outlierCount);
    _payloadBuilder->writeParam("excludes", _excludeCount);
    _payloadBuilder->writeParam("overruns", _overrunCount);
    _payloadBuilder->writeGroupEnd();
    _payloadBuilder->writeGroupStart("duration");
    _payloadBuilder->writeParam("total", _sumDuration);
    _payloadBuilder->writeParam("average", averageDuration);
    _payloadBuilder->writeParam("max", _maxDuration);
    _payloadBuilder->writeGroupEnd();
    _payloadBuilder->writeGroupStart("analysis");
    _payloadBuilder->writeParam("smoothValue", _smoothValue);
    _payloadBuilder->writeParam("derivative", _derivative);
    _payloadBuilder->writeParam("smoothDerivative", _smoothDerivative);
    _payloadBuilder->writeParam("smoothAbsDerivative", _smoothAbsDerivative);
    _payloadBuilder->writeGroupEnd();
    BatchWriter::prepareFlush();
}

void ResultWriter::resetCounters() {
    BatchWriter::resetCounters();
    _excludeCount = 0L;
    _flowCount = 0L;
    _maxDuration = 0L;
    _outlierCount = 0L;
    _overrunCount = 0L;
    _peakCount = 0L;
    _sumDuration = 0L;
    _eventServer->publish(this, Topic::TimeOverrun, LONG_FALSE);
    _eventServer->publish(this, Topic::Exclude, LONG_FALSE);
}

void ResultWriter::setIdleFlushRate(long rate) {
    if (rate != _idleFlushRate) {
        _idleFlushRate = rate;
    }
    setDesiredFlushRate(rate);
}

void ResultWriter::setNonIdleFlushRate(long rate) {
    if (rate != _nonIdleFlushRate) {
        _nonIdleFlushRate = rate;
    }
}

void ResultWriter::update(Topic topic, const char* payload) {
    // we only listen to topics with numerical values, so conversion should work
    switch (topic) {
    case Topic::IdleRate:
        update(topic, convertToLong(payload, FLUSH_RATE_IDLE));
        return;
    case Topic::NonIdleRate:
        update(topic, convertToLong(payload, FLUSH_RATE_INTERESTING));
        return;
    default:
        update(topic, convertToLong(payload, 0));
    }
}

void ResultWriter::update(Topic topic, long payload) {
    long rate = limit(payload, 0L, LONG_MAX);
    switch (topic) {
    case Topic::IdleRate:
        setIdleFlushRate(rate);
        return;
    case Topic::NonIdleRate:
        setNonIdleFlushRate(rate);
        return;
    case Topic::ProcessTime:
        addDuration(payload);
        return;
    default:
        BatchWriter::update(topic, payload);
    }
}
