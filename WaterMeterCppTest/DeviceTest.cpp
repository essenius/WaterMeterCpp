// Copyright 2021-2022 Rik Essenius
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

#include "TestEventClient.h"
#include "../WaterMeterCpp/Device.h"

namespace WaterMeterCppTest {
    using WaterMeter::Device;
    
    // ReSharper disable once CyclomaticComplexity -- caused by EXPECT macros
    TEST(DeviceTest, deviceTest1) {
        EventServer eventServer;
        TestEventClient stackListener(&eventServer), heapListener(&eventServer);
        eventServer.subscribe(&heapListener, Topic::FreeHeap);
        eventServer.subscribe(&stackListener, Topic::FreeStack);
        uxTaskGetStackHighWaterMarkReset();
        Device device(&eventServer);
        // ensure that nothing breaks when we call it too early
        device.reportHealth();
        EXPECT_EQ(0, stackListener.getCallCount()) << "Stack not called as not yet initialized";
        EXPECT_EQ(1, heapListener.getCallCount()) << "Heap called as does not need task handles";
        EXPECT_STREQ("32000", heapListener.getPayload()) << "Free heap is 32k";

        device.begin(xTaskGetCurrentTaskHandle(), nullptr, nullptr);
        device.reportHealth();

        EXPECT_EQ(3, stackListener.getCallCount()) << "Stack called three times";
        EXPECT_EQ(1, heapListener.getCallCount()) << "Heap not called again - 29k does not pass lower limit of 25k";
        EXPECT_STREQ("33558182", stackListener.getPayload()) << "Free stack for sampler is 2-3750 (due to nullptr handles)";
        device.reportHealth();
        EXPECT_EQ(4, stackListener.getCallCount()) << "Stack called again - different value";
        EXPECT_EQ(1, heapListener.getCallCount()) << "Heap not called again - 26k does not pass new lower limit of 25k";
        EXPECT_STREQ("1564", stackListener.getPayload()) << "Free stack is 1564";

        device.reportHealth();
        EXPECT_EQ(5, stackListener.getCallCount()) << "Stack called yet again";
        EXPECT_EQ(2, heapListener.getCallCount()) << "Heap called again, passed lower limit of 25k";
        EXPECT_STREQ("1628", stackListener.getPayload()) << "Free stack is 1628";
        device.reportHealth();
        EXPECT_EQ(5, stackListener.getCallCount()) << "Stack not called as still the same";
        EXPECT_EQ(2, heapListener.getCallCount()) << "Heap not called as not below limit/threshold (20k is edge case)";
        EXPECT_STREQ("23000", heapListener.getPayload()) << "Free heap is 23k";
        device.reportHealth();
        EXPECT_EQ(5, stackListener.getCallCount()) << "Stack still not called as still the same";
        EXPECT_EQ(3, heapListener.getCallCount()) << "Heap called as below low threshold";
        EXPECT_STREQ("17000", heapListener.getPayload()) << "Free heap is 17k";
        device.reportHealth();
        EXPECT_EQ(6, stackListener.getCallCount()) << "Stack called as different value";
        EXPECT_EQ(4, heapListener.getCallCount()) << "Heap called as below low threshold";
        EXPECT_STREQ("1500", stackListener.getPayload()) << "Free stack is 1500";
        EXPECT_STREQ("14000", heapListener.getPayload()) << "Free heap is 14000";
        device.reportHealth();
        EXPECT_EQ(4, heapListener.getCallCount()) << "Heap not called as same value (even below low threshold)";
        device.reportHealth();
        EXPECT_EQ(5, heapListener.getCallCount()) << "Heap called as below low threshold again";
        EXPECT_STREQ("11000", heapListener.getPayload()) << "Free heap is 11000";
        device.reportHealth();
        EXPECT_EQ(6, heapListener.getCallCount()) << "Heap called as large enough difference (up)";
        EXPECT_STREQ("32000", heapListener.getPayload()) << "Free heap is 32000";
    }
}
