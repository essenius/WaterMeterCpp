// Copyright 2021-2023 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

// Mock implementation of the ESP library for unit testing (not targeting the ESP32)

// Disabling warnings caused by mimicking existing interfaces
// ReSharper disable CppInconsistentNaming
// ReSharper disable CppMemberFunctionMayBeStatic
// ReSharper disable CppParameterNeverUsed

#ifndef HEADER_ESP
#define HEADER_ESP
#define WIN32_LEAN_AND_MEAN
#define ARDUINO_ISR_ATTR
#include <cstdint>
#include <string>
#include <SafeCString.h>

// ReSharper disable once CppUnusedIncludeDirective -- added on purpose
#include <freertos/freeRTOS.h>

using byte = uint8_t;

constexpr uint8_t INPUT = 0x0;
constexpr uint8_t OUTPUT = 0x1;
constexpr uint8_t INPUT_PULLUP = 0x2;

constexpr uint8_t LOW = 0x0;
constexpr uint8_t HIGH = 0x1;

constexpr uint8_t LED_BUILTIN = 13;

#define PI 3.1415926535897932384626433832795

// emulation of the relevant Arduino capabilities

/**
 * \brief // Mock implementation of the Esp class for unit testing (not targeting the ESP32)
 */
class Esp {
public:
    void restart() { _heapCount = -1; }
    int getFreeHeap();
private:
    int _heapCount = -1;
};

extern Esp ESP;

/**
 * \brief Mock implementation of the HardwareSerial class for unit testing (not targeting the ESP32)
 */
class HardwareSerial {
public:
    int available();
    void begin(int speed);
    void print(const char* input);
    void println(const char* input);
    char read();
    void setTimeout(long timeout);

    // test support (i.e. don't use in production code)
    void clearInput();
    void clearOutput();
    const char* getOutput();
    void setInput(const char* input);

    template <typename... Arguments>
    int printf(const char* format, Arguments ... arguments) {
        const int length = SafeCString::pointerSprintf(_printBufferPointer, _printBuffer, format, arguments...);
        _printBufferPointer += length;
        return length;
    }

    static constexpr size_t PrintbufferSize = 4096;

private:
    char _printBuffer[PrintbufferSize] = {};
    char* _printBufferPointer = _printBuffer;
    static constexpr size_t InputBufferSize = 100;
    char _inputBuffer[InputBufferSize] = {};
    char* _inputBufferPointer = nullptr;
};

extern HardwareSerial Serial;

template <typename... Arguments>
int redirectPrintf(const char* format, Arguments ... arguments) {
    return Serial.printf(format, arguments...);
}

inline const char* getPrintOutput() {
    return Serial.getOutput();
}

inline void clearPrintOutput() {
    Serial.clearOutput();
}

// GPIO functions

uint8_t digitalRead(uint8_t pin);
void digitalWrite(uint8_t pin, uint8_t value);
void pinMode(uint8_t pin, uint8_t mode);

// for testing only
uint8_t getPinMode(uint8_t pin);

// Timing functions

void configTime(int i, int i1, const char* str, const char* text);
void delay(int delay);
void delayMicroseconds(unsigned long delay);
unsigned long micros();
unsigned long millis();

/**
 * \brief Testing: shift the micros() clock
 * \param shift micros to shift 
 */
void shiftMicros(long long shift);

/**
 * \brief Testing: set whether the millis() clock is real time or not
 * \param on true: realtime, false: not realtime
 */
void setRealTime(bool on);

// delay() is often used to give time for other tasks. Whwn testing, we often run sequential and then this interferes

/**
 * \brief Testing: disable delay() (to be able to run code sequentially which is normally run in tasks)
 * \param disable whether to disable delay() or not
 */
void disableDelay(bool disable);

// Logging

enum class LogLevel { Error = 1, Warning, Info, Debug, Verbose };

const char* toString(LogLevel level);

extern LogLevel minLogLevel;

inline void setLogLevel(const LogLevel level) {
    minLogLevel = level;
}

template <typename... Arguments>
void log_printf(const LogLevel level, const char* format, Arguments ... arguments) {
    if (minLogLevel >= level) {
        Serial.printf("[%s] ", toString(level));
        Serial.printf(format, arguments...);
        Serial.print("\n");
    }
}

template <typename... Arguments>
void log_e(const char* format, Arguments ... arguments) {
    log_printf(LogLevel::Error, format, arguments...);
}

template <typename... Arguments>
void log_w(const char* format, Arguments ... arguments) {
    log_printf(LogLevel::Warning, format, arguments...);
}

template <typename... Arguments>
void log_i(const char* format, Arguments ... arguments) {
    log_printf(LogLevel::Info, format, arguments...);
}

template <typename... Arguments>
void log_d(const char* format, Arguments ... arguments) {
    log_printf(LogLevel::Debug, format, arguments...);
}

template <typename... Arguments>
void log_v(const char* format, Arguments ... arguments) {
    log_printf(LogLevel::Verbose, format, arguments...);
}

struct hw_timer_t {
    uint8_t group;
    uint8_t num;
};

hw_timer_t* timerBegin(uint8_t num, uint16_t divider, bool countUp);
inline void timerAttachInterrupt(hw_timer_t* timer, void (*fn)(), bool edge) {}
inline void timerAlarmWrite(hw_timer_t* timer, uint64_t alarm_value, bool autoreload) {}
void timerAlarmEnable(hw_timer_t* timer);
void timerEnd(hw_timer_t* timer);
#endif
