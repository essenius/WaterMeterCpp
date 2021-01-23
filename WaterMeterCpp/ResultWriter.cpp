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

#include "ResultWriter.h"
#include <string.h>
#include <limits.h>

ResultWriter::ResultWriter(Storage *storage, int measureIntervalMicros) : BatchWriter(storage->getIdleRate(ResultWriter::FLUSH_RATE_IDLE), 'S') {
	_measureIntervalMicros = measureIntervalMicros;
	_storage = storage;
	_idleFlushRate = _desiredFlushRate;
}

void ResultWriter::addMeasurement(int value, int duration, FlowMeter* result) {
	newMessage();
	_measure = value;
	_result = result;
	_sumDuration += duration;
	if (_maxDuration < duration) {
		_maxDuration = duration;
	}
	if (duration > _measureIntervalMicros) {
		_overrunCount++;
	}
	if (_result->hasFlow()) {
		_flowCount++;
		_sumAmplitude += _result->getAmplitude();
		_sumLowPassOnHighPass += _result->getLowPassOnHighPass();
	}
	if (_result->isOutlier()) {
		_outlierCount++;
	}
	if (_result->hasDrift()) {
		_driftCount++;
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

char* ResultWriter::getHeader() {
	writeParam("Measure,Flows,Peaks,SumAmplitude,SumLPonHP,LowPassFast,LowPassSlow,LPonHP,Outliers,Drifts,Excludes,AvgDuration,MaxDuration,Overruns");
	return BatchWriter::getHeader();
}

long ResultWriter::getIdleFlushRate() {
	return _idleFlushRate;
}

long ResultWriter::getNonIdleFlushRate() {
	return _nonIdleFlushRate;
}

bool ResultWriter::needsFlush(bool endOfFile) {
	// We set the flush rate regardless of whether we still need to write something. This can end an idle batch early.
	bool isInteresting = _flowCount > 0 || _excludeCount > 0;
	if (isInteresting) {
	    _flushRate = _nonIdleFlushRate;
	}
	return BatchWriter::needsFlush(endOfFile);
}

void ResultWriter::prepareFlush() {
	int averageDuration = (int)((_sumDuration * 10 / _messageCount + 5) / 10);
	if (_flushRate == 1) {
		writeParam(_measure);
	}
	else {
		writeParam(_messageCount);
	}
	writeParam(_flowCount);
	writeParam(_peakCount);
	writeParam(_sumAmplitude);
	writeParam(_sumLowPassOnHighPass);
	writeParam(_result->getLowPassFast());
	writeParam(_result->getLowPassSlow());
	writeParam(_result->getLowPassOnHighPass());
	writeParam(_outlierCount);
	writeParam(_driftCount);
	writeParam(_excludeCount);
    writeParam(averageDuration);
    writeParam(_maxDuration);
	writeParam(_overrunCount);
	BatchWriter::prepareFlush();
}

void ResultWriter::resetCounters() {
	BatchWriter::resetCounters();
	_driftCount = 0L;
	_excludeCount = 0L;
	_flowCount = 0L;
	_outlierCount = 0L;
	_overrunCount = 0L;
	_peakCount = 0L;
	_sumAmplitude = 0.0;
	_sumLowPassOnHighPass = 0.0;
	_sumDuration = 0L;
	_maxDuration = 0L;
}

void ResultWriter::setIdleFlushRate(long rate) {
	_idleFlushRate = rate;
	setDesiredFlushRate(rate);
    _storage->setIdleRate(rate);
}

void ResultWriter::setIdleFlushRate(char* rateString) {
	unsigned int rate = convertToLong(rateString, FLUSH_RATE_IDLE, 0, LONG_MAX);
	setIdleFlushRate(rate);
}

void ResultWriter::setNonIdleFlushRate(long rate) {
	_nonIdleFlushRate = rate;
    _storage->setNonIdleRate(rate);
}

void ResultWriter::setNonIdleFlushRate(char* rateString) {
	unsigned int rate = convertToLong(rateString, FLUSH_RATE_INTERESTING, 0, LONG_MAX);
	setNonIdleFlushRate(rate);
}
