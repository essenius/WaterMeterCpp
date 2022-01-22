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

#ifndef HEADER_PAYLOADBUILDER
#define HEADER_PAYLOADBUILDER
#include "Clock.h"

class PayloadBuilder {
public:
    void begin();
    void initialize(const char prefix = '{');
    bool isAlmostFull() const;
    int remainingSize() const;
    const char* toString() const;
    void updatePosition();
    void writeArrayEnd();
    void writeArrayStart(const char* label);
    void writeDelimiter(char delimiter);
    void writeGroupEnd();
    void writeGroupStart(const char* label);
    void writeLabel(const char* label);
    void writeTimestamp(Timestamp timestampIn);
    void writeTimestampParam(const char* label, Timestamp timestampIn);
    void writeText(const char* text);

    template <class T>
    void writeArrayValue(T value) {
        if (_needsDelimiter) {
            writeDelimiter(',');
        }
        writeString(value);
        updatePosition();
        _needsDelimiter = true;
    }

    template <class T>
    void writeParam(const char* label, T value) {
        writeLabel(label);
        writeString(value);
        updatePosition();
        _needsDelimiter = true;
    }

private:
    static constexpr int RESULT_BUFFER_SIZE = 512;
    static constexpr int NUMBER_BUFFER_SIZE = 16;
    static constexpr int RESULT_BUFFER_MARGIN = 20;

    void writeString(const char* input);
    void writeString(float input);
    void writeString(int input);
    void writeString(long input);
    void writeString(unsigned long input);
    void writeString(uint32_t input);

    char* _currentPosition = _resultBuffer;
    bool _needsDelimiter = false;
    char _numberBuffer[NUMBER_BUFFER_SIZE] = { 0 };
    char _resultBuffer[RESULT_BUFFER_SIZE] = { 0 };
};

#endif
