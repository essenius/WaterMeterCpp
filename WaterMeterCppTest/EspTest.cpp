// Copyright 2022 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#include "pch.h"

#include "CppUnitTest.h"
#include <ESP.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
    TEST_CLASS(EspTest) {
public:

    TEST_METHOD(espRealTimeTest) {
        setRealTime(true);
        disableDelay(false);
        auto currentTime = micros();
        delayMicroseconds(1000);
        auto newTime = micros();
        Assert::IsTrue(newTime - currentTime >= 1000, L"delayMicroseconds() worked");
        shiftMicros(0);
        Assert::IsTrue(micros() - currentTime < 100, L"shiftMicros() undid DelayMicroseconds");
        currentTime = micros();
        delay(2);
        Assert::IsTrue(micros() - currentTime >= 2000, L"delay() worked");

    }

    TEST_METHOD(espSerialTest) {
        Serial.begin(115200);
        Serial.setTimeout(1000);
        Serial.print(toString(LogLevel::Info));
        Serial.print(toString(LogLevel::Debug));
        Serial.print(toString(LogLevel::Verbose));
        Serial.print(toString(static_cast<LogLevel>(0)));
        Serial.printf("%d%s\n", 23, toString(LogLevel::Warning));
        Serial.println(toString(LogLevel::Error));
        Assert::AreEqual("IDV23W\nE\n", Serial.getOutput(), "Output ok");
        Serial.clearOutput();
        Assert::IsFalse(Serial.available(), L"No data available");
        auto character = Serial.read();
        Assert::AreEqual('\0', character, L"nothing to read read");
        Serial.setInput("hello");
        Assert::IsTrue(Serial.available(), L"Data available");
        character = Serial.read();
        Assert::AreEqual('h', character, L"First character read");
        for (int i = 0; i < HardwareSerial::PRINTBUFFER_SIZE - 5; i++) {
            Serial.print("x");
        }
        Serial.printf("123456");
        Assert::AreEqual("1234", Serial.getOutput() + HardwareSerial::PRINTBUFFER_SIZE - 5, "No buffer overrun");
        Assert::AreEqual<size_t>(HardwareSerial::PRINTBUFFER_SIZE - 1, strlen(Serial.getOutput()), L"length OK");
    }

    };
}