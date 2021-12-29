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
		TestEventClient stackListener("stack", &eventServer), heapListener("heap", &eventServer);
		eventServer.subscribe(&heapListener, Topic::FreeHeap);
		eventServer.subscribe(&stackListener, Topic::FreeStack);

		Device device(&eventServer);
		device.begin();
		Assert::AreEqual(1, stackListener.getCallCount(), L"Stack called once");
		Assert::AreEqual(1, heapListener.getCallCount(), L"Heap called once");
		Assert::AreEqual("1500", stackListener.getPayload(), L"Free stack is 1500");
		Assert::AreEqual("25000", heapListener.getPayload(), L"Free heap is 25000");
		device.reportHealth();
		Assert::AreEqual(2, stackListener.getCallCount(), L"Stack called again");
		Assert::AreEqual(1, heapListener.getCallCount(), L"Heap not called again");
		Assert::AreEqual("1564", stackListener.getPayload(), L"Free stack is 1564");
		device.reportHealth();
		Assert::AreEqual(3, stackListener.getCallCount(), L"Stack called third time");
		Assert::AreEqual(2, heapListener.getCallCount(), L"Heap called second time");
		Assert::AreEqual("1628", stackListener.getPayload(), L"Free stack is 1638");
		Assert::AreEqual("30000", heapListener.getPayload(), L"Free heap is 30000");
		device.reportHealth();
		Assert::AreEqual(4, stackListener.getCallCount(), L"Stack called third time");
		Assert::AreEqual(3, heapListener.getCallCount(), L"Heap called second time");
		Assert::AreEqual("1500", stackListener.getPayload(), L"Free stack is 1500");
		Assert::AreEqual("25000", heapListener.getPayload(), L"Free heap is 25000");
	}
	};
}
