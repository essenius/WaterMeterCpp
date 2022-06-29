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
#include <freertos/freeRTOS.h>

namespace WaterMeterCppTest {
    
    class FreeRtosTest : public testing::Test {
    public:
        QueueHandle_t queue[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
        char buffer[10] = "x";
    };

    TEST_F(FreeRtosTest, freeRtosTest1) {
        uxQueueReset();
        for (int i = 0; i < 5; i++) {
            queue[i] = xQueueCreate(5, 2);
            EXPECT_NE(nullptr, queue[i]) << "Not null";
        }
        EXPECT_EQ(nullptr, xQueueCreate(5, 2)) << "6th returns 0";
        EXPECT_EQ(20ul, uxQueueSpacesAvailable(queue[0])) << "queue 0 has 20 spaces left at start";
        EXPECT_EQ(0ul, uxQueueMessagesWaiting(queue[0])) << "queue 0 has 0 waiting message at start";
        EXPECT_EQ(pdFALSE, xQueueReceive(queue[0], buffer, 0)) << "Nothing in the queue";
        EXPECT_EQ(pdFALSE, xQueueSendToFront(queue[0], buffer, 0)) << "Not implemented";
        EXPECT_EQ(pdTRUE, xQueueSendToBack(queue[0], buffer, 0)) << "Item sent";
        EXPECT_EQ(19ul, uxQueueSpacesAvailable(queue[0])) << "queue 0 has 19 spaces left";
        EXPECT_EQ(1ul, uxQueueMessagesWaiting(queue[0])) << "queue 0 has 1 waiting message";
        buffer[0] = '\0';
        EXPECT_EQ(pdTRUE, xQueueReceive(queue[0], buffer, 0)) << "Item read";
        EXPECT_STREQ("x", buffer) << "read item has right value";
        EXPECT_EQ(20ul, uxQueueSpacesAvailable(queue[0])) << "queue 0 has 20 spaces left after read";
        EXPECT_EQ(0ul, uxQueueMessagesWaiting(queue[0])) << "queue 0 has 0 waiting message after read";

        EXPECT_EQ(pdTRUE, xQueueSendToBack(queue[1], buffer, 0)) << "Item sent to queue 1";
        EXPECT_EQ(pdTRUE, xQueueSendToBack(queue[3], buffer, 0)) << "Item sent to queue 3";
        uxQueueReset();
        EXPECT_NE(nullptr, xQueueCreate(5, 2)) << "Can create new queue after reset";
        uxQueueReset();
    }
}
