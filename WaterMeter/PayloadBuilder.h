// Copyright 2021-2024 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

// Creates a payload in JSON format to be sent over MQTT. Not using JSON libraries as we want to limit the size a bit.

#ifndef HEADER_PAYLOAD_BUILDER
#define HEADER_PAYLOAD_BUILDER
#include <cstdint>

#include "Clock.h"

namespace WaterMeter {
    class PayloadBuilder {
    public:
        explicit PayloadBuilder(Clock* theClock = nullptr);
        void begin();
        void initialize(char prefix = '{');
        bool isAlmostFull() const;
        int getRemainingSize() const;
        const char* toString() const;
        void updatePosition();
        void writeArrayEnd();
        void writeArrayStart(const char* label);

        template <class T>
        void writeArrayValue(T value) {
            if (_needsDelimiter) {
                writeDelimiter(',');
            }
            writeString(value);
            updatePosition();
            _needsDelimiter = true;
        }

        void writeDelimiter(char delimiter);
        void writeGroupEnd();
        void writeGroupStart(const char* label);
        void writeLabel(const char* label);

        template <class T>
        void writeParam(const char* label, T value) {
            writeLabel(label);
            writeString(value);
            updatePosition();
            _needsDelimiter = true;
        }

        void writeTimestamp(Timestamp timestampIn);
        void writeTimestampParam(const char* label, Timestamp timestampIn);
        void writeText(const char* text);

    private:
        static constexpr int NumberBufferSize = 16;
        static constexpr int ResultBufferMargin = 20;
        static constexpr int ResultBufferSize = 512;

        void writeString(const char* input);
        void writeString(double input);
        void writeString(int input);
        void writeString(long input);
        void writeString(uint32_t input);
        void writeString(unsigned long input);
        void writeString(SensorSample input);

        Clock* _clock;
        char* _currentPosition = _resultBuffer;
        bool _needsDelimiter = false;
        char _numberBuffer[NumberBufferSize] = { 0 };
        char _resultBuffer[ResultBufferSize] = { 0 };
    };
}
#endif
