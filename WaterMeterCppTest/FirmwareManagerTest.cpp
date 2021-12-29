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
#include "../WaterMeterCpp/FirmwareManager.h"
#include "../WaterMeterCpp/NetMock.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
	TEST_CLASS(FirmwareManagerTest) {
public:

	TEST_METHOD(FirmwareManagerUpdateAvailableTest) {
		EventServer eventServer;
		FirmwareManager manager(&eventServer);
		WiFiClient client;
		manager.begin(&client, "http://localhost/images", "001122334455");

		TestEventClient infoListener("log", &eventServer);
		TestEventClient errorListener("log", &eventServer);
		eventServer.subscribe(&infoListener, Topic::Info);
		eventServer.subscribe(&errorListener, Topic::Error);

		// Successful check, same version
	    HTTPClient::ReturnValue = 200;
		Assert::IsFalse(manager.updateAvailableFor(1), L"No update for version 1");
		Assert::AreEqual(0, infoListener.getCallCount(), L"No info");
		Assert::AreEqual(0, errorListener.getCallCount(), L"No error");

		// Failed check
		HTTPClient::ReturnValue = 400;
		Assert::IsFalse(manager.updateAvailableFor(1), L"No update for version 1");
		Assert::AreEqual(0, infoListener.getCallCount(), L"No info");
		Assert::AreEqual(1, errorListener.getCallCount(), L"Error");
		Assert::AreEqual("Firmware version check failed with response code 400\n", errorListener.getPayload(), L"Error correct");

		// Successful check, other version
		HTTPClient::ReturnValue = 200;
		Assert::IsTrue(manager.updateAvailableFor(2), L"update for version 2 available");
		Assert::AreEqual(1, infoListener.getCallCount(), L"Info called");
		Assert::AreEqual(1, errorListener.getCallCount(), L"No new error");
		Assert::AreEqual("Current firmware version: 2; available version: 1\n", infoListener.getPayload(), L"Info correct");
	}

	TEST_METHOD(FirmwareManagerUpdateTest) {
		EventServer eventServer;
		FirmwareManager manager(&eventServer);
		WiFiClient client;
		manager.begin(&client, "http://localhost/images/", "001122334455");

		TestEventClient infoListener("log", &eventServer);
		TestEventClient errorListener("log", &eventServer);
		eventServer.subscribe(&infoListener, Topic::Info);
		eventServer.subscribe(&errorListener, Topic::Error);

		// check succeeds, update succeeds (but doesn't reboot, obviously)
		HTTPClient::ReturnValue = 200;
		HTTPUpdate::ReturnValue = HTTP_UPDATE_OK;
	    manager.tryUpdateFrom(2);
		Assert::AreEqual(2, infoListener.getCallCount(), L"info on success");
		Assert::AreEqual(0, errorListener.getCallCount(), L"No error on success");
		Assert::AreEqual("Firmware not updated (2/0): OK\n", infoListener.getPayload(), L"warning message");

		// check fails
		HTTPClient::ReturnValue = 400;
		manager.tryUpdateFrom(2);
		Assert::AreEqual(2, infoListener.getCallCount(), L"no info on check failure");
		Assert::AreEqual(1, errorListener.getCallCount(), L"Erro on check failure");
		Assert::AreEqual("Firmware version check failed with response code 400\n", errorListener.getPayload(), L"Error correct");

		// check succeeds and update fails
		HTTPClient::ReturnValue = 200;
		HTTPUpdate::ReturnValue = HTTP_UPDATE_FAILED;
		manager.tryUpdateFrom(2);
		Assert::AreEqual(3, infoListener.getCallCount(), L"new info on update failure");
		Assert::AreEqual("Current firmware version: 2; available version: 1\n", infoListener.getPayload(), "info on update failure OK");
		Assert::AreEqual(2, errorListener.getCallCount(), L"error on update failure");
		Assert::AreEqual("Firmware update failed (0): OK\n", errorListener.getPayload(), L"error message on update failure OK");

		HTTPUpdate::ReturnValue = HTTP_UPDATE_OK;
		// check succeeds but no update needed
		manager.tryUpdateFrom(1);
		Assert::AreEqual(3, infoListener.getCallCount(), L"no info on check without update");
		Assert::AreEqual(2, errorListener.getCallCount(), L"no error on check without update");

	}

	};
}
