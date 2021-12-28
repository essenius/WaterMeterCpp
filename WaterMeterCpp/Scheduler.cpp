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

#include "Scheduler.h"
#include "BatchWriter.h"
//#include "ResultWriter.h"
//#include "MeasurementWriter.h"

Scheduler::Scheduler(EventServer* eventServer, BatchWriter* measureWriter, BatchWriter* resultWriter) : EventClient("Scheduler", eventServer) {
	_writer[MEASURE_ID] = measureWriter;
	_writer[RESULT_ID] = resultWriter;
}

// The optimal number of writes is the average rate per sample, rounded down
int Scheduler::optimalWriteCount(int round) {
	float result = 0.0;
	for (int i = 0; i <= round - 1; i++) {
		result += 1.0f / _writer[i]->getFlushRate();
	}
	return (int)result;
}

bool Scheduler::processOutput() {
	bool resultWritten = false;
	bool hasWritten = false;
	bool delayedFlush = false;
	int writes = 0;
	for (int i = 0; i < WRITER_COUNT; i++) {
		// optimize the write schedule. If we need to write twice or more in one sample, see if we can postpone.
		int optimalWrites = optimalWriteCount(i);
		bool needsFlush = _writer[i]->needsFlush();
		if (needsFlush && (writes <= optimalWrites)) {
			writes++;
			if (i == 1) {
				resultWritten = true;
			}
			hasWritten = true;
			_eventServer->publish(this, i == 0 ? Topic::Measurement : Topic::Result, _writer[i]->getMessage());
			_writer[i]->flush();
		}
		else {
			delayedFlush |= needsFlush;
		}
	}
	if (_resultWritten != resultWritten) {
		_eventServer->publish<long>(this, Topic::ResultWritten, resultWritten);
		_resultWritten = resultWritten;
	}
	if (_delayedFlush != delayedFlush) {
		_eventServer->publish<long>(this, Topic::DelayedFlush, delayedFlush);
		_delayedFlush = delayedFlush;
	}
	return hasWritten;
}
