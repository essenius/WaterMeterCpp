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

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifdef ARDUINO
  #include <Arduino.h>
#else
  #include <stdio.h>
  #include "Arduino.h"
#endif

#include <string.h>
#include <stdlib.h>
#include "BatchWriter.h"

BatchWriter::BatchWriter(long flushRate, char startSymbol) {
    _desiredFlushRate = flushRate;
    _startSymbol = startSymbol;
    _numberBuffer[0] = '\0';
    flush();
    resetCounters();
}

long BatchWriter::convertToLong(char* stringParam, long defaultValue, long min, long max) {
    if (strcmp(stringParam, "DEFAULT") == 0) {
        return defaultValue;
    }
    long convertedValue = strtoul(stringParam, 0, 10);
    if (convertedValue > max) {
        return max;
    }
    if (convertedValue < min) {
        return min;
    }
    return convertedValue;
}

unsigned short int BatchWriter::crc16_update(unsigned short int  crc, byte a) {
    int i;
    crc ^= a;
    for (i = 0; i < 8; ++i) {
        if (crc & 1) {
            crc = (crc >> 1) ^ 0xA001;
        }
        else {
            crc = (crc >> 1);
        }
    }
    return crc;
}

void BatchWriter::flush() {
    _printBuffer[0] = _startSymbol;
    _printBuffer[1] = '\0';
    _flushWaiting = false;
}

unsigned int BatchWriter::getCrc(const char* input) {
    unsigned short int  crc = 0;
    for (size_t i = 0; i < strlen(input); i++) {
        crc = crc16_update(crc, input[i]);
    }
    return crc;
}

long BatchWriter::getFlushRate() {
    return _flushRate;
}

char* BatchWriter::getHeader() {
    writeParam("CRC");
    return _printBuffer;
};

char* BatchWriter::getMessage() {
    return _printBuffer;
}

// We flush the log every nth time, n being the flush rate. This puts less strain on the receiving end.
// Returns whether a write action took place, so we can ensure writes are in different loops.

bool BatchWriter::needsFlush(bool endOfFile) {
    if (_flushWaiting) {
        return true;
    }
    if (_flushRate == 0 || _messageCount == 0 || !_canFlush) { 
        return false; 
    }
    bool bufferAlmostFull = strlen(_printBuffer) > PRINT_BUFFER_SIZE - PRINT_BUFFER_MARGIN;
    if (_messageCount % _flushRate != 0 && !endOfFile && !bufferAlmostFull) {
        return false; 
    }
    prepareFlush();
    return true;
}

bool BatchWriter::newMessage() {
    if (_flushRate == 0) return false;
    _messageCount++;
    return true;
}

void BatchWriter::prepareFlush() {
    int crc = getCrc(_printBuffer);
    writeParam(crc);
    // Now we can start recording the next round. The buffer isn't necessarily flushed in the same cycle as it's prepared.
    resetCounters();
    _flushWaiting = true;
}

void BatchWriter::resetCounters() {
    _messageCount = 0;
    _flushRate = _desiredFlushRate;
}

void BatchWriter::setDesiredFlushRate(long flushRate) {
    _desiredFlushRate = flushRate;
    // Immediately apply if we are starting a new round, or aren't logging
    // Otherwise it will be done at the next reset.
    if (_messageCount == 0 || _flushRate == 0) {
        _flushRate = _desiredFlushRate;
    }
}

long BatchWriter::getDesiredFlushRate() {
    return _desiredFlushRate;
}

void BatchWriter::setCanFlush(bool canFlush) {
    _canFlush = canFlush;
}

char* BatchWriter::toString(float input, unsigned char precision) {
    dtostrf(input, 1, precision, _numberBuffer);
    if (strchr(_numberBuffer, '.') == 0) return _numberBuffer;
    // we have a fraction. Clean up overprecision.
    char* endPointer = _numberBuffer + strlen(_numberBuffer);
    while (*(--endPointer) == '0');
    if (*endPointer != '.') {
        endPointer++;
    }
    *endPointer = 0;
    return _numberBuffer;    
}

char* BatchWriter::toString(int number) {
    sprintf(_numberBuffer, "%i", number);
    return _numberBuffer;
}

char* BatchWriter::toString(long number) {
    sprintf(_numberBuffer, "%ld", number);
    return _numberBuffer;
}

// Do not call the following methods if newMessage has returned false.

void BatchWriter::writeParam(const char* value) {
    strcat(_printBuffer, ",");
    strcat(_printBuffer, value);
}

void BatchWriter::writeParam(float value) {
    writeParam(toString(value, 2));
}

void BatchWriter::writeParam(int value) {
    writeParam(toString(value));
}

void BatchWriter::writeParam(long value) {
    writeParam(toString(value));
}
