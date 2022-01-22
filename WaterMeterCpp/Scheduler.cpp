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

/*
#include "Scheduler.h"

Scheduler::Scheduler(EventServer* eventServer, DataQueue* dataQueue,  Aggregator* measureWriter, Aggregator* resultWriter) :
    EventClient("Scheduler", eventServer), _dataQueue(dataQueue) {
    _aggregator[MEASURE_ID] = measureWriter;
    _aggregator[RESULT_ID] = resultWriter;
}

// The optimal number of writes is the average rate per sample, rounded down
int Scheduler::optimalWriteCount(int round) {
    float result = 0.0;
    for (int i = 0; i <= round - 1; i++) {
        result += 1.0f / static_cast<float>(_aggregator[i]->getFlushRate());
    }
    return static_cast<int>(result);
}

void Scheduler::processOutputSimple() {
    for (int i = 0; i < WRITER_COUNT; i++) {
        if (_aggregator[i]->needsFlush()) {
            _dataQueue->send(_aggregator[i]->getPayload());
            _aggregator[i]->flush();
        }
    }
}

bool Scheduler::processOutput() {
    bool resultWritten = false;
    bool hasWritten = false;
    bool delayedFlush = false;
    int writes = 0;
    for (int i = 0; i < WRITER_COUNT; i++) {
        // optimize the write schedule. If we need to write twice or more in one sample, see if we can postpone.
        const int optimalWrites = optimalWriteCount(i);
        const bool needsFlush = _aggregator[i]->needsFlush();
        if (needsFlush && (writes <= optimalWrites)) {
            writes++;
            if (i == 1) {
                resultWritten = true;
            }
            hasWritten = true;
            _eventServer->publish(this, i == 0 ? Topic::Measurement : Topic::Result, _aggregator[i]->getMessage());
            _aggregator[i]->flush();
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
*/