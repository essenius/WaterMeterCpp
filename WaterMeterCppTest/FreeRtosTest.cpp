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
#include <freertos/freeRTOS.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
    TEST_CLASS(FreeRtosTest) {
public:
    QueueHandle_t queue[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
    char buffer[10] = "x";
    TEST_METHOD(freeRtosTest1) {
        uxQueueReset();
        for (int i = 0; i < 5; i++) {
            queue[i] = xQueueCreate(5, 2);
                Assert::IsNotNull(queue[i], L"Not null");
        }
        Assert::IsNull(xQueueCreate(5, 2), L"6th returns 0");
        Assert::AreEqual(20ul, uxQueueSpacesAvailable(queue[0]), L"queue 0 has 20 spaces left at start");
        Assert::AreEqual(0ul, uxQueueMessagesWaiting(queue[0]), L"queue 0 has 0 waiting message at start");
        Assert::AreEqual(pdFALSE, xQueueReceive(queue[0], buffer, 0), L"Nothing in the queue");
        Assert::AreEqual(pdFALSE, xQueueSendToFront(queue[0], buffer, 0), L"Not implemented");
        Assert::AreEqual(pdTRUE, xQueueSendToBack(queue[0], buffer, 0), L"Item sent");
        Assert::AreEqual(19ul, uxQueueSpacesAvailable(queue[0]), L"queue 0 has 19 spaces left");
        Assert::AreEqual(1ul, uxQueueMessagesWaiting(queue[0]), L"queue 0 has 1 waiting message");
        buffer[0] = '\0';
        Assert::AreEqual(pdTRUE, xQueueReceive(queue[0], buffer, 0), L"Item read");
        Assert::AreEqual("x", buffer, L"read item has right value");
        Assert::AreEqual(20ul, uxQueueSpacesAvailable(queue[0]), L"queue 0 has 20 spaces left after read");
        Assert::AreEqual(0ul, uxQueueMessagesWaiting(queue[0]), L"queue 0 has 0 waiting message after read");

        Assert::AreEqual(pdTRUE, xQueueSendToBack(queue[1], buffer, 0), L"Item sent to queue 1");
        Assert::AreEqual(pdTRUE, xQueueSendToBack(queue[3], buffer, 0), L"Item sent to queue 3");
        uxQueueReset();
        Assert::IsNotNull(xQueueCreate(5, 2), L"Can create new queue after reset");
        uxQueueReset();

    }
    };
}