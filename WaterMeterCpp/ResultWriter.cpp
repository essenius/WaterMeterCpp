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
#include <limits.h>
#include "ResultWriter.h"

ResultWriter::ResultWriter(EventServer* eventServer, TimeServer* timeServer, PayloadBuilder* payloadBuilder, int measureIntervalMicros) :
    BatchWriter("ResultWriter", eventServer, timeServer, payloadBuilder) {
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
    _result = result;
    if (_result->isOutlier()) {
        _outlierCount++;
    }
    if (_result->getPeak() > 0) {
        _peakCount++;
    }
    if (_result->areAllExcluded()) {
        _excludeCount = _messageCount;
    }
    else if (result->isExcluded()) {
        _excludeCount++;
    }
}

void ResultWriter::begin() {
    _eventServer->subscribe(this, Topic::Connected);
    _eventServer->subscribe(this, Topic::IdleRate);
    _eventServer->subscribe(this, Topic::NonIdleRate);
    _eventServer->subscribe(this, Topic::ProcessTime);
    _eventServer->subscribe(this, Topic::Peak);
    BatchWriter::begin(FLUSH_RATE_IDLE);
    // This is the only time the desired rates get published from here.
    _eventServer->publish(this, Topic::IdleRate, _idleFlushRate);
    _eventServer->publish(this, Topic::NonIdleRate, _nonIdleFlushRate);
}

bool ResultWriter::needsFlush(bool endOfFile) {
    // We set the flush rate regardless of whether we still need to write something. This can end an idle batch early.
    bool isInteresting = _flowCount > 0 || _excludeCount > 0;
    if (isInteresting) {
        _flushRatePublisher.update(_nonIdleFlushRate);
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
    int averageDuration = (int)((_sumDuration * 10 / _messageCount + 5) / 10);
    _payloadBuilder->writeParam("lastValue", _measure);
    _payloadBuilder->writeGroupStart("summaryCount");
    _payloadBuilder->writeParam("samples", _messageCount);
    _payloadBuilder->writeParam("peaks", _peakCount);
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
    //_payloadBuilder->writeParam("sumAmplitude", _sumAmplitude);
    //_payloadBuilder->writeParam("sumLowPassOnHighPass", _sumLowPassOnHighPass);
    _payloadBuilder->writeParam("smoothValue", _result->getSmoothValue());
    _payloadBuilder->writeParam("derivative", _result->getDerivative());
    _payloadBuilder->writeParam("smoothDerivative", _result->getSmoothDerivative());
    _payloadBuilder->writeGroupEnd();
    BatchWriter::prepareFlush();
}

void ResultWriter::resetCounters() {
    BatchWriter::resetCounters();
    _driftCount = 0L;
    _excludeCount = 0L;
    _flowCount = 0L;
    _maxDuration = 0L;
    _outlierCount = 0L;
    _overrunCount = 0L;
    _peakCount = 0L;
    _sumAmplitude = 0.0;
    _sumDuration = 0L;
    _sumLowPassOnHighPass = 0.0;
    _eventServer->publish(this, Topic::TimeOverrun, LONG_FALSE);
    _eventServer->publish(this, Topic::Flow, LONG_FALSE);
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
    // The only string payloads are for new flush rates
    update(topic, convertToLong(payload, topic == Topic::IdleRate ? FLUSH_RATE_IDLE : FLUSH_RATE_INTERESTING));
}

void ResultWriter::update(Topic topic, long payload) {
    long rate = limit(payload, 0L, LONG_MAX);
    switch (topic) {
    case Topic::IdleRate:
        setIdleFlushRate(rate);
        break;
    case Topic::NonIdleRate:
        setNonIdleFlushRate(rate);
        break;
    case Topic::Connected:
        _canFlush = payload == LONG_TRUE;
        break;
    case Topic::ProcessTime:
        addDuration(payload);
    default:
        BatchWriter::update(topic, payload);;
    }
}
