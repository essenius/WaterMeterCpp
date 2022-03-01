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

// Mock implementation for unit testing (not targeting the ESP32)

// Disabling warnings caused by mimicking existing interfaces
// ReSharper disable CppInconsistentNaming
// ReSharper disable CppMemberFunctionMayBeStatic

#ifndef ESP32

#ifndef HEADER_ARDUINOMOCK
#define HEADER_ARDUINOMOCK

#define WIN32_LEAN_AND_MEAN
#include <cstdint>
#include <string>

typedef uint8_t byte;

constexpr uint8_t INPUT = 0x0;
constexpr uint8_t OUTPUT = 0x1;
constexpr uint8_t INPUT_PULLUP = 0x2;
constexpr uint8_t LOW = 0x0;
constexpr uint8_t HIGH = 0x1;
constexpr uint8_t LED_BUILTIN = 13;

// emulation of Arduino capabilities

class Esp {
public:
	void restart() {}
};

extern Esp ESP;


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
	static constexpr int PRINTBUFFER_SIZE = 4096;
	char _printBuffer[PRINTBUFFER_SIZE] = {};
	static constexpr int INPUTBUFFER_SIZE = 80;
	char _inputBuffer[INPUTBUFFER_SIZE] = {};
	char* _bufferPointer = nullptr;

};

extern HardwareSerial Serial;

void configTime(int i, int i1, const char* str, const char* text);

void delay(int delay);
void delayMicroseconds(int delay);
//char* dtostrf(float value, signed char width, unsigned char precision, char* buffer);
uint8_t digitalRead(uint8_t pin);
void digitalWrite(uint8_t pin, uint8_t value);
unsigned long micros();
unsigned long millis();
void pinMode(uint8_t pin, uint8_t mode);

// for testing only
uint8_t getPinMode(uint8_t pin);
void shiftMicros(long long shift);

// testing only too
enum LogLevel { error = 1, warning, info, debug, verbose };

const char* toString(LogLevel level);

extern LogLevel minLogLevel;

inline void setLogLevel(const LogLevel level) { minLogLevel = level; };

template <typename... Arguments>
void log_printf(const LogLevel level, const char* format, Arguments ... arguments) {
	if (minLogLevel >= level) {
		Serial.printf("[%s] ", toString(level));
		Serial.printf(format, arguments...);
		Serial.print("\n");
	}
}

template <typename... Arguments>
void log_e(const char* format, Arguments ... arguments) { log_printf(error, format, arguments...); }

template <typename... Arguments>
void log_w(const char* format, Arguments ... arguments) { log_printf(warning, format, arguments...); }

template <typename... Arguments>
void log_i(const char* format, Arguments ... arguments) { log_printf(info, format, arguments...); }

template <typename... Arguments>
void log_d(const char* format, Arguments ... arguments) { log_printf(debug, format, arguments...); }

template <typename... Arguments>
void log_v(const char* format, Arguments ... arguments) { log_printf(verbose, format, arguments...); }

#endif

#endif