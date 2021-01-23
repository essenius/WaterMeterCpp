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

#ifndef ARDUINO

#define _CRT_SECURE_NO_WARNINGS
#include "Arduino.h"
#include <stdio.h>
#include <chrono>
#include <string.h>

HardwareSerial Serial;
auto startTime = std::chrono::high_resolution_clock::now();

char* dtostrf(float value, signed char width, unsigned char precision, char* buffer) {
		char fmt[20];
		sprintf(fmt, "%%%d.%df", width, precision);
		sprintf(buffer, fmt, value);
		return buffer;
}

const uint8_t PIN_COUNT = 13;
uint8_t pinValue[PIN_COUNT];
uint8_t pinModeValue[PIN_COUNT];

uint8_t getPinMode(uint8_t pin) {
	return pinModeValue[pin];
}

void pinMode(uint8_t pin, uint8_t mode) {
	pinModeValue[pin] = mode;
}

void digitalWrite(uint8_t pin, uint8_t value) {
	pinValue[pin] = value;
}

uint8_t digitalRead(uint8_t pin) {
	return pinValue[pin];
}

long long microShift = 0;

void shiftMicros(long long shift) {
	microShift = shift;
}

unsigned long micros(void) {
	auto now = std::chrono::high_resolution_clock::now() ;
	return std::chrono::duration_cast<std::chrono::microseconds>(now - startTime).count() + microShift;
}


void delay(int delay) {}

int HardwareSerial::available() {
	return strlen(_bufferPointer);
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

char* HardwareSerial::getOutput() {
	return _printBuffer;
}

void HardwareSerial::print(const char* input) {
	strcat_s(_printBuffer, PRINTBUFFER_SIZE, input);
};

void HardwareSerial::println(const char* input) {
	print(input);
	strcat_s(_printBuffer, PRINTBUFFER_SIZE, "\n");
}

char HardwareSerial::read() {
	if (*_bufferPointer == 0) return 0;
	return *(_bufferPointer++);
}

void HardwareSerial::setInput(const char* input) {
	strcpy_s(_inputBuffer, INPUTBUFFER_SIZE, input);
	_bufferPointer = _inputBuffer;
}

void HardwareSerial::setTimeout(long timeout) {}

#endif
