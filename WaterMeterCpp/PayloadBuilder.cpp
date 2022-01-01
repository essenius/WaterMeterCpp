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

#include "PayloadBuilder.h"
#include <cstring>
#include <cstdio>

// higher level functions. See also the template function writeParam in .h

void PayloadBuilder::begin() {
    _resultBuffer[0] = 0;
    _currentPosition = _resultBuffer;
}

void PayloadBuilder::initialize() {
    begin();
    writeDelimiter('{');
    _needsDelimiter = false;
}

void PayloadBuilder::writeArrayEnd() {
    writeDelimiter(']');
    _needsDelimiter = true;
}

void PayloadBuilder::writeArrayStart(const char* label) {
    writeLabel(label);
    writeDelimiter('[');
    _needsDelimiter = false;
}

void PayloadBuilder::writeGroupEnd() {
    writeDelimiter('}');
    _needsDelimiter = true;
}

void PayloadBuilder::writeGroupStart(const char* label) {
    if (_needsDelimiter) {
        writeDelimiter(',');
    }
    writeString(label);
    writeDelimiter(':');
    writeDelimiter('{');
    _needsDelimiter = false;
}

void PayloadBuilder::writeLabel(const char* label) {
    if (_needsDelimiter) {
        writeDelimiter(',');
    }
    writeString(label);
    writeDelimiter(':');
}

const char* PayloadBuilder::toString() const {
    return _resultBuffer;
}

// low level functions 

bool PayloadBuilder::isAlmostFull() const {
    return remainingSize() < RESULT_BUFFER_MARGIN;
}

int PayloadBuilder::remainingSize() const {
    return RESULT_BUFFER_SIZE - static_cast<int>(strlen(_resultBuffer)) - 1;
}

void PayloadBuilder::updatePosition() {
    _currentPosition = _resultBuffer + strlen(_resultBuffer);
}

void PayloadBuilder::writeDelimiter(const char delimiter) {
    *(_currentPosition++) = delimiter;
    *_currentPosition = 0;
}

void PayloadBuilder::writeString(const char* input) {
    strcat(_resultBuffer, "\"");
    strcat(_resultBuffer, input);
    strcat(_resultBuffer, "\"");
    updatePosition();
}

void PayloadBuilder::writeString(const float input) {
    sprintf(_currentPosition, "%.2f", input);
    updatePosition();
    // clean up any overprecision
    while (*(--_currentPosition) == '0') {}
    if (*_currentPosition != '.') {
        _currentPosition++;
    }
    *_currentPosition = 0;
}

void PayloadBuilder::writeString(int input) {
    sprintf(_currentPosition, "%i", input);
    updatePosition();
}

void PayloadBuilder::writeString(long input) {
    sprintf(_currentPosition, "%ld", input);
    updatePosition();
}


void PayloadBuilder::writeString(unsigned long input) {
    sprintf(_currentPosition, "%lu", input);
    updatePosition();
}
