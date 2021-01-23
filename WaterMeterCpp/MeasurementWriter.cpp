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


// We need to use sprintf etc. on the Arduino (sprintf_s doesn't work there), so suppressing the warnings
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <stdio.h>
#endif

#include "MeasurementWriter.h"

MeasurementWriter::MeasurementWriter(Storage *storage) : BatchWriter(storage->getLogRate(DEFAULT_FLUSH_RATE), 'M') {
    //setDesiredFlushRate(DEFAULT_FLUSH_RATE);
    _storage = storage;
}

void MeasurementWriter::addMeasurement(int measure, int duration) {
    // Only record measurements if flush rate > 0 and we can flush
    if (newMessage() && _canFlush) {
        writeParam(toString(measure));
    }
    else {
        // If we can't flush (broken connection), discard what we have in the buffer
        flush();
        resetCounters();
    }
}

char* MeasurementWriter::getHeader() {
    for (int i = 0; i < getFlushRate(); i++) {
        sprintf(_numberBuffer, "M%i", i);
        writeParam(_numberBuffer);
    }
    return BatchWriter::getHeader();
}

void MeasurementWriter::setDesiredFlushRate(long flushRate) {
    BatchWriter::setDesiredFlushRate(flushRate);
    _storage->setLogRate((unsigned char)flushRate);
}

void MeasurementWriter::setDesiredFlushRate(char* desiredFlushRate) {
    unsigned char desiredRate = convertToLong(desiredFlushRate, DEFAULT_FLUSH_RATE, 0L, MAX_FLUSH_RATE);
    setDesiredFlushRate(desiredRate);
}
