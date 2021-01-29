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

#ifdef ARDUINO
  #include <Arduino.h>
#else
  #include "Arduino.h"
#endif

#include "SerialScheduler.h"
#include "ResultWriter.h"
#include "MeasurementWriter.h"

SerialScheduler::SerialScheduler(SerialDriver *serialDriver, BatchWriter* measureWriter, BatchWriter* resultWriter, BatchWriter* infoWriter, int (* freeMemory)()) {
	_serialDriver = serialDriver;
	_writer[MEASURE] = measureWriter;
	_writer[RESULT] = resultWriter;
	_writer[INFO] = infoWriter;
    _freeMemory = freeMemory;
}

bool SerialScheduler::hasDelayedFlush() {
    return _delayedFlush;
}

bool SerialScheduler::wasResultWritten() {
    return _resultWritten;
}

bool SerialScheduler::awaitingResponse() {
    return _lastSent > 0L;
}

// Don't call this function in the middle of a run. It will overwrite the print buffer.
void SerialScheduler::writeHeaders() {
    if (_isConnected && _lastSent == 0L) {
        _serialDriver->println(_writer[MEASURE]->getHeader());
        _writer[MEASURE]->flush();
        _serialDriver->println(_writer[RESULT]->getHeader());
        _writer[RESULT]->flush();
        _lastSent = micros();
    }
}

// The optimal number of writes is the average rate per sample, rounded down
int SerialScheduler::optimalWriteCount(int round) {
    float result = 0.0;
    for (int i = 0; i <= round-1; i++) {
        result += 1.0f / _writer[i]->getFlushRate();
    }
    return (int)result;
}

bool SerialScheduler::processOutput() {
    _resultWritten = false;
    bool hasWritten = false;
    if (isConnected() && _lastSent == 0L) {
        _delayedFlush = false;
        int writes = 0;
        for (int i = 0; i < WRITER_COUNT; i++) {
            // optimize the write schedule. If we need to write twice or more in one sample, see if we can postpone.
            int optimalWrites = optimalWriteCount(i);
            bool needsFlush = _writer[i]->needsFlush();
            if (needsFlush && (writes <= optimalWrites)) {
                writes++;
                if (i == 1) {
                    _resultWritten = true;
                }
                hasWritten = true;
                _serialDriver->println(_writer[i]->getMessage());
                _writer[i]->flush();
                _lastSent = micros();
            }
            else {
                _delayedFlush |= needsFlush;
            }
        }
    }
    return hasWritten;
}

bool SerialScheduler::isConnected() {
    if (_isConnected && (_lastSent > 0L)) {
        if (micros() - _lastSent > RESPONSE_TIMEOUT_MICROS) { 
            _isConnected = false;
        }
    }
    setCanFlush();
    return _isConnected;
}

void SerialScheduler::setCanFlush() {
    for (int i = 0; i < WRITER_COUNT; i++) {
        _writer[i]->setCanFlush(_isConnected);
    }
}

bool SerialScheduler::processInput() {
    if (_serialDriver->inputAvailable()) {
        char* command = _serialDriver->getCommand();
        char* param = _serialDriver->getParameter();

        if (param != 0) {
            switch (command[0]) {
                case 'L':
                    ((MeasurementWriter *)_writer[MEASURE])->setDesiredFlushRate(param);
                    break;
                case 'I':
                    ((ResultWriter *)_writer[RESULT])->setIdleFlushRate(param);
                    break;
                case 'N':
                    ((ResultWriter *)_writer[RESULT])->setNonIdleFlushRate(param);
                    break;
                default:
                    break;
            }
        } else {
            switch (command[0]) {
            case 'A':
                _isConnected = true;
                _lastSent = 0;
                setCanFlush();
                _writer[INFO]->newMessage();
                _writer[INFO]->writeParam("Version,0.4");
                _writer[INFO]->needsFlush(true);
                _serialDriver->println(_writer[INFO]->getMessage());
                _writer[INFO]->flush();
                writeHeaders();
                break;
            case 'M':
                _lastSent = 0;
                _writer[INFO]->newMessage();
                _writer[INFO]->writeParam("MemoryFree");
                _writer[INFO]->writeParam(_freeMemory());
                break;
            case 'Q':
                _isConnected = false;
                _delayedFlush = false;
                break;
            case 'R':
                _lastSent = 0;
                _writer[INFO]->newMessage();
                _writer[INFO]->writeParam("Rates:current");
                _writer[INFO]->writeParam("log");
                _writer[INFO]->writeParam(_writer[MEASURE]->getFlushRate());
                _writer[INFO]->writeParam("result");
                _writer[INFO]->writeParam(_writer[RESULT]->getFlushRate());
                _writer[INFO]->writeParam("desired");
                _writer[INFO]->writeParam("log");
                _writer[INFO]->writeParam(_writer[MEASURE]->getDesiredFlushRate());
                _writer[INFO]->writeParam("idle");
                _writer[INFO]->writeParam(((ResultWriter*)_writer[RESULT])->getIdleFlushRate());
                _writer[INFO]->writeParam("non-idle");
                _writer[INFO]->writeParam(((ResultWriter*)_writer[RESULT])->getNonIdleFlushRate());
                _writer[INFO]->writeParam("desired");
                _writer[INFO]->writeParam(_writer[RESULT]->getDesiredFlushRate());
                break;
            case 'Y':
                _lastSent = 0;
            default:
                break;
            }
        }
        _serialDriver->clearInput();
        return true;
    }
    return false;
}
