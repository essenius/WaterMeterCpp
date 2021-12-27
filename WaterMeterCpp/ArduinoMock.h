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

// Mock implementation for unit testing (not targeting the ESP32)

#ifndef ESP32

#ifndef HEADER_ARDUINOMOCK
#define HEADER_ARDUINOMOCK

#include <stdint.h>
#include <string>

typedef uint8_t byte;

const uint8_t INPUT = 0x0;
const uint8_t OUTPUT = 0x1;
const uint8_t INPUT_PULLUP = 0x2;

const uint8_t LOW = 0x0;
const uint8_t HIGH = 0x1;

const uint8_t LED_BUILTIN = 13;

// emulation of Arduino capabilities

class HardwareSerial {
public:
	int available();
	void begin(int speed);
	void print(const char* input);
	void printf(const char* format, ...);
	void println(const char* input);
	char read();
	void setTimeout(long timeout);
	// test support, not in original Serial object (i.e. don't use in production code)
	void clearInput();
	void clearOutput();
	const char* getOutput();
	void setInput(const char* input);
private:
	static constexpr int PRINTBUFFER_SIZE = 1024;
	char _printBuffer[PRINTBUFFER_SIZE] = {};
	static constexpr int INPUTBUFFER_SIZE = 80;
	char _inputBuffer[INPUTBUFFER_SIZE] = {};
	char* _bufferPointer = nullptr;

};

extern HardwareSerial Serial;

char* dtostrf(float value, signed char width, unsigned char precision, char* buffer);
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
uint8_t digitalRead(uint8_t pin);
unsigned long micros(void);
void delay(int delay);

// for testing only
uint8_t getPinMode(uint8_t pin);
void shiftMicros(long long shift);

#endif

#endif