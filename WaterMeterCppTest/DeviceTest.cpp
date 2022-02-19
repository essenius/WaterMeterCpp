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

#include "pch.h"

#include <regex>

#include "CppUnitTest.h"
#include "TestEventClient.h"
#include "../WaterMeterCpp/Device.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
    TEST_CLASS(DeviceTest) {
    public:
        TEST_METHOD(deviceTest1) {
            EventServer eventServer;
            TestEventClient stackListener(&eventServer), heapListener(&eventServer);
            eventServer.subscribe(&heapListener, Topic::FreeHeap);
            eventServer.subscribe(&stackListener, Topic::FreeStack);
            TaskHandle_t handle1 = nullptr;
            TaskHandle_t handle2 = nullptr;

            Device device(&eventServer);
            device.begin(xTaskGetCurrentTaskHandle(), handle1, handle2);
            device.reportHealth();

            Assert::AreEqual(3, stackListener.getCallCount(), L"Stack called three times");
            Assert::AreEqual(1, heapListener.getCallCount(), L"Heap called once");
            Assert::AreEqual("33558182", stackListener.getPayload(), L"Free stack for sampler is 2-1500");
            Assert::AreEqual("32000", heapListener.getPayload(), L"Free heap is 32000");
            device.reportHealth();
            Assert::AreEqual(4, stackListener.getCallCount(), L"Stack called again - different value");
            Assert::AreEqual(2, heapListener.getCallCount(), L"Heap called again - 29k passes lower limit of 30k");
            Assert::AreEqual("1564", stackListener.getPayload(), L"Free stack is 1564");
            Assert::AreEqual("29000", heapListener.getPayload(), L"Free heap is 29000");

            device.reportHealth();
            Assert::AreEqual(5, stackListener.getCallCount(), L"Stack called yet again");
            Assert::AreEqual(2, heapListener.getCallCount(), L"Heap not called again - did not pass new lower limit of 25k");
            Assert::AreEqual("1628", stackListener.getPayload(), L"Free stack is 1628");
            device.reportHealth();
            Assert::AreEqual(5, stackListener.getCallCount(), L"Stack not called as still the same");
            Assert::AreEqual(3, heapListener.getCallCount(), L"Heap called again, passed lower limit of 25k");
            Assert::AreEqual("23000", heapListener.getPayload(), L"Free heap is 23000");
            device.reportHealth();
            Assert::AreEqual(5, stackListener.getCallCount(), L"Stack still not called as still the same");
            Assert::AreEqual(3, heapListener.getCallCount(), L"Heap not called as same value");
            device.reportHealth();
            Assert::AreEqual(6, stackListener.getCallCount(), L"Stack called as different value");
            Assert::AreEqual(4, heapListener.getCallCount(), L"Heap called as below low threshold");
            Assert::AreEqual("1500", stackListener.getPayload(), L"Free stack is 1500");
            Assert::AreEqual("17000", heapListener.getPayload(), L"Free heap is 17000");
            device.reportHealth();
            Assert::AreEqual(5, heapListener.getCallCount(), L"Heap called as below low threshold");
            Assert::AreEqual("14000", heapListener.getPayload(), L"Free heap is 14000");
            device.reportHealth();
            Assert::AreEqual(5, heapListener.getCallCount(), L"Heap not called as same value (even below low threshold)");
            device.reportHealth();
            Assert::AreEqual(6, heapListener.getCallCount(), L"Heap called as below low threshold");
            Assert::AreEqual("11000", heapListener.getPayload(), L"Free heap is 11000");
            device.reportHealth();
            Assert::AreEqual(7, heapListener.getCallCount(), L"Heap called as large enough difference (up)");
            Assert::AreEqual("32000", heapListener.getPayload(), L"Free heap is 32000");
        }
    };
}
