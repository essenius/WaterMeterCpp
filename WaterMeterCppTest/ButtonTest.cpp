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
#include <regex>

#include "TestEventClient.h"
#include "../WaterMeterCpp/Button.h"

namespace WaterMeterCppTest {

    TEST(ButtonTest, buttonTest1) {
        constexpr uint8_t PORT = 34;
        digitalWrite(PORT, HIGH);
        EventServer eventServer;
        TestEventClient buttonListener(&eventServer);
        eventServer.subscribe(&buttonListener, Topic::ButtonPushed);

        ChangePublisher<uint8_t> publisher(&eventServer, Topic::ButtonPushed);
        Button button(&publisher, 34);
        button.begin();
        EXPECT_EQ(1, buttonListener.getCallCount()) << "Initial value was set after begin";
        EXPECT_EQ(HIGH, publisher.get()) << "Initial value is HIGH";
        button.check();
        EXPECT_EQ(HIGH, publisher.get()) << "Initial value remains after first check";
        digitalWrite(PORT, LOW);
        delay(1);
        button.check();
        EXPECT_EQ(1, buttonListener.getCallCount()) << "Initial value still not changed - awaiting end of bouncing after LOW";
        digitalWrite(PORT, HIGH);
        delay(1);
        button.check();
        EXPECT_EQ(1, buttonListener.getCallCount()) << "Initial value still not changed - awaiting end of bouncing after HIGH";
        digitalWrite(PORT, LOW);
        delay(1);
        button.check();
        EXPECT_EQ(1, buttonListener.getCallCount()) << "Initial value still not changed - awaiting end of bouncing adter second LOW before timeout";
        delay(11);
        button.check();
        EXPECT_EQ(2, buttonListener.getCallCount()) << "Topic triggered after timeout";
        EXPECT_EQ(LOW, publisher.get()) << "Value changed to LOW after timeout";
    }
}