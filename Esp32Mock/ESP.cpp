// Copyright 2021-2022 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

// Mock implementation for unit testing (not targeting the ESP32)

// Disabling warnings caused by mimicking existing interfaces
// ReSharper disable CppInconsistentNaming 
// ReSharper disable CppMemberFunctionMayBeStatic
// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppParameterMayBeConst
// ReSharper disable CppParameterNeverUsed

#include <ESP.h>
#include <chrono>
#include <cstdarg>
#include "../WaterMeterCpp/SafeCString.h"

Esp ESP;

// Simulate changes as well as staying the same
int Esp::getFreeHeap() {
    _heapCount++;
    if (_heapCount <= 6) return 32000L - _heapCount * 3000L;
    if (_heapCount == 7) return 14000;
    _heapCount = -1;
    return 11000L;
}

// GPIO functions

constexpr uint8_t PIN_COUNT = 36;
uint8_t pinValue[PIN_COUNT];
uint8_t pinModeValue[PIN_COUNT];

uint8_t digitalRead(uint8_t pin) {
    return pinValue[pin];
}

void digitalWrite(uint8_t pin, uint8_t value) {
    pinValue[pin] = value;
}

uint8_t getPinMode(uint8_t pin) {
    return pinModeValue[pin];
}

void pinMode(uint8_t pin, uint8_t mode) {
    pinModeValue[pin] = mode;
}

// Time functions

unsigned long Micros = 0;
unsigned long MicrosSteps = 50;
bool RealTimeOn = false;
auto startTime = std::chrono::high_resolution_clock::now();
bool DisableDelay = false;

long long microShift = 0;

void configTime(int i, int i1, const char* str, const char* text) {}

void shiftMicros(long long shift) {
    microShift = shift;
}

void delayMicroseconds(int delay) {
    if (RealTimeOn) {
        microShift += delay;
    } else {
        Micros += delay;
    }
}

void disableDelay(bool disable) {
    DisableDelay = disable;
}

void delay(int delay) {
    if (DisableDelay) return;
    if (RealTimeOn) {
        microShift += delay * 1000LL;
    } else {
        Micros += delay * 1000UL;
    }
}

void setRealTime(bool on) {
    RealTimeOn = on;
    if (RealTimeOn) {
        startTime = std::chrono::high_resolution_clock::now();
    } else {
        Micros = 0;
    }
}

unsigned long millis() {
    return micros() / 1000;
}

unsigned long micros() {
    if (RealTimeOn) {
        const auto now = std::chrono::high_resolution_clock::now();
        return static_cast<unsigned long>(std::chrono::duration_cast<std::chrono::microseconds>(now - startTime).count() +
            microShift);
    }
    Micros += MicrosSteps;
    return Micros;
}

// Serial class

HardwareSerial Serial;

int HardwareSerial::available() {
    return static_cast<int>(strlen(_inputBufferPointer));
}

void HardwareSerial::begin(int speed) {
    clearInput();
    clearOutput();
}

void HardwareSerial::clearInput() {
    _inputBuffer[0] = 0;
    _inputBufferPointer = _inputBuffer;
}

void HardwareSerial::clearOutput() {
    _printBuffer[0] = 0;
    _printBufferPointer = _printBuffer;
}

const char* HardwareSerial::getOutput() {
    return _printBuffer;
}

void HardwareSerial::print(const char* input) {
    safeStrcat(_printBuffer, input);
    _printBufferPointer = _printBuffer + strlen(_printBuffer);
}

void HardwareSerial::println(const char* input) {
    print(input);
    safeStrcat(_printBuffer, "\n");
    _printBufferPointer = _printBuffer + strlen(_printBuffer);
}

char HardwareSerial::read() {
    if (*_inputBufferPointer == 0) return 0;
    return *_inputBufferPointer++;
}

void HardwareSerial::setInput(const char* input) {
    safeStrcpy(_inputBuffer, input);
    _inputBufferPointer = _inputBuffer;
}

void HardwareSerial::setTimeout(long timeout) {}

const char* toString(LogLevel level) {
    switch (level) {
    case LogLevel::Error: return "E";
    case LogLevel::Warning: return "W";
    case LogLevel::Info: return "I";
    case LogLevel::Debug: return "D";
    case LogLevel::Verbose: return "V";
    }
    return nullptr;
}

LogLevel minLogLevel = LogLevel::Info;

/* char* dtostrf(float value, signed char width, unsigned char precision, char* buffer) {
        char fmt[20];
        safeSprintf(fmt, "%%%d.%df", width, precision);
        sprintf(buffer, fmt, value);
        return buffer;
}*/