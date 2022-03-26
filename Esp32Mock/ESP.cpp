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
#include <cstdio>
#include <chrono>
#include <cstdarg>
#include <iostream>
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

HardwareSerial Serial;

char PrintBuffer[2048];
char* PrintBufferPointer = PrintBuffer;

auto startTime = std::chrono::high_resolution_clock::now();

long long microShift = 0;

void configTime(int i, int i1, const char* str, const char* text) {}

/* char* dtostrf(float value, signed char width, unsigned char precision, char* buffer) {
		char fmt[20];
		safeSprintf(fmt, "%%%d.%df", width, precision);
		sprintf(buffer, fmt, value);
		return buffer;
}*/

void shiftMicros(long long shift) {
    microShift = shift;
}

void delayMicroseconds(int delay) { microShift += delay; }

void delay(int delay) { microShift += delay * 1000LL; }

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

unsigned long millis() {
    const auto now = std::chrono::high_resolution_clock::now();
    return static_cast<unsigned long>(std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count() +
        microShift / 1000);
}

unsigned long micros() {
    const auto now = std::chrono::high_resolution_clock::now();
    return static_cast<unsigned long>(std::chrono::duration_cast<std::chrono::microseconds>(now - startTime).count() +
        microShift);
}

void pinMode(uint8_t pin, uint8_t mode) {
    pinModeValue[pin] = mode;
}

int HardwareSerial::available() {
    return static_cast<int>(strlen(_bufferPointer));
}

void HardwareSerial::begin(int speed) {
    clearInput();
    clearOutput();
}

void HardwareSerial::clearInput() {
    _inputBuffer[0] = 0;
    _bufferPointer = _inputBuffer;
}

void HardwareSerial::clearOutput() {
    _printBuffer[0] = 0;
}

const char* HardwareSerial::getOutput() {
    return _printBuffer;
}

void HardwareSerial::print(const char* input) {
    safeStrcat(_printBuffer, input);
}

void HardwareSerial::printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsprintf_s(_printBuffer + strlen(_printBuffer), PRINTBUFFER_SIZE - strlen(_printBuffer), format, args);
    std::cout << _printBuffer;
    va_end(args);
}

void HardwareSerial::println(const char* input) {
    print(input);
    safeStrcat(_printBuffer, "\n");
}

char HardwareSerial::read() {
    if (*_bufferPointer == 0) return 0;
    return *(_bufferPointer++);
}

void HardwareSerial::setInput(const char* input) {
    safeStrcpy(_inputBuffer, input);
    _bufferPointer = _inputBuffer;
}

void HardwareSerial::setTimeout(long timeout) {}

const char* toString(LogLevel level) {
    switch (level) {
    case error: return "E";
    case warning: return "W";
    case info: return "I";
    case debug: return "D";
    case verbose: return "V";
    }
    return nullptr;
}

LogLevel minLogLevel = info;
