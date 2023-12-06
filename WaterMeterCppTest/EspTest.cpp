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

#include "gtest/gtest.h"
#include <ESP.h>

namespace WaterMeterCppTest {
    
    TEST(EspTest, espRealTimeTest) {
        setRealTime(true);
        disableDelay(false);
        auto currentTime = micros();
        delayMicroseconds(1000);
        const auto newTime = micros();
        EXPECT_TRUE(newTime - currentTime >= 1000) << "delayMicroseconds() worked";
        shiftMicros(0);
        EXPECT_TRUE(micros() - currentTime < 100) << "shiftMicros() undid DelayMicroseconds";
        currentTime = micros();
        delay(2);
        EXPECT_TRUE(micros() - currentTime >= 2000) << "delay() worked";
    }

    TEST(EspTest, espSerialTest) {
        Serial.begin(115200);
        Serial.setTimeout(1000);
        Serial.print(toString(LogLevel::Info));
        Serial.print(toString(LogLevel::Debug));
        Serial.print(toString(LogLevel::Verbose));
        Serial.print(toString(static_cast<LogLevel>(0)));
        Serial.printf("%d%s\n", 23, toString(LogLevel::Warning));
        Serial.println(toString(LogLevel::Error));
        EXPECT_STREQ("IDV23W\nE\n", Serial.getOutput()) << "Output ok";
        Serial.clearOutput();
        EXPECT_FALSE(Serial.available()) << "No data available";
        auto character = Serial.read();
        EXPECT_EQ('\0', character) << "nothing to read read";
        Serial.setInput("hello");
        EXPECT_TRUE(Serial.available()) << "Data available";
        character = Serial.read();
        EXPECT_EQ('h', character) << "First character read";
        for (unsigned int i = 0; i < HardwareSerial::PrintbufferSize - 5; i++) {
            Serial.print("x");
        }
        Serial.printf("123456");
        EXPECT_STREQ("1234", Serial.getOutput() + HardwareSerial::PrintbufferSize - 5) << "No buffer overrun";
        EXPECT_EQ(HardwareSerial::PrintbufferSize - 1, strlen(Serial.getOutput())) << "length OK";
    }
}

