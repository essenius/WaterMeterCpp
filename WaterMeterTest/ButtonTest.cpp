// Copyright 2022-2024 Rik Essenius
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
#include <regex>

#include "TestEventClient.h"
#include "Button.h"

namespace WaterMeterCppTest {
    using WaterMeter::Button;
    using WaterMeter::ChangePublisher;
    TEST(ButtonTest, scriptTest) {
        constexpr uint8_t Port = 34;
        // State in rest is HIGH
        digitalWrite(Port, HIGH);
        EventServer eventServer;
        TestEventClient buttonListener(&eventServer);
        eventServer.subscribe(&buttonListener, Topic::ButtonPushed);
        ChangePublisher<uint8_t> publisher(&eventServer, Topic::ButtonPushed);
        EXPECT_EQ(LOW, publisher.get()) << "Default value of publisher is LOW";
        Button button(&publisher, 34);
        button.begin();
        EXPECT_EQ(0, buttonListener.getCallCount()) << "Value not changed (port value HIGH is publisher value LOW)";
        button.check();
        EXPECT_EQ(LOW, publisher.get()) << "Initial value remains after first check";
        // Simulate push
        digitalWrite(Port, LOW);
        delay(1);
        button.check();
        EXPECT_EQ(0, buttonListener.getCallCount()) << "Initial value still not changed - awaiting end of bouncing after LOW";
        // Simulate bounce 
        digitalWrite(Port, HIGH);
        delay(1);
        button.check();
        EXPECT_EQ(0, buttonListener.getCallCount()) << "Initial value still not changed - awaiting end of bouncing after HIGH";
        // back to pressed
        digitalWrite(Port, LOW);
        delay(1);
        button.check();
        EXPECT_EQ(0, buttonListener.getCallCount()) << "Initial value still not changed - awaiting end of bouncing after second LOW before timeout";
        delay(11);
        button.check();
        EXPECT_EQ(1, buttonListener.getCallCount()) << "Topic triggered after bounce timeout";
        EXPECT_EQ(HIGH, publisher.get()) << "Value changed to HIGH after bounce timeout";
    }
}