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
        static WiFiClient Client;
        static EventServer EventServer;
        static TestEventClient InfoListener;
        static TestEventClient ErrorListener;

        TEST_CLASS_INITIALIZE(firmwareClassInitialize) {
            EventServer.subscribe(&InfoListener, Topic::Info);
            EventServer.subscribe(&ErrorListener, Topic::Error);
        }

        TEST_METHOD_INITIALIZE(firmwareMethodInitialize) {
            InfoListener.reset();
            ErrorListener.reset();
        }

        TEST_METHOD(firmwareManagerUpdateAvailableTest) {
            FirmwareManager manager(&EventServer);
            manager.begin(&Client, "http://localhost/images", "001122334455");


            // Successful check, same version
            HTTPClient::ReturnValue = 200;
            Assert::IsFalse(manager.updateAvailableFor("0.1.1"), L"No update for version 0.1.1");
            Assert::AreEqual(0, InfoListener.getCallCount(), L"No info");
            Assert::AreEqual(0, ErrorListener.getCallCount(), L"No error");

            // Failed check
            HTTPClient::ReturnValue = 400;
            Assert::IsFalse(manager.updateAvailableFor("0.1.1"), L"No update for version 0.1.1");
            Assert::AreEqual(0, InfoListener.getCallCount(), L"No info");
            Assert::AreEqual(1, ErrorListener.getCallCount(), L"Error");
            Assert::AreEqual("Firmware version check failed with response code 400\n", ErrorListener.getPayload(),
                             L"Error correct");

            // Successful check, other version
            HTTPClient::ReturnValue = 200;
            Assert::IsTrue(manager.updateAvailableFor("0.1.2"), L"update for version 0.1.2 available");
            Assert::AreEqual(1, InfoListener.getCallCount(), L"Info called");
            Assert::AreEqual(1, ErrorListener.getCallCount(), L"No new error");
            Assert::AreEqual("Current firmware version: '0.1.2'; available version: '0.1.1'\n", InfoListener.getPayload(),
                             L"Info correct");
        }

        TEST_METHOD(FirmwareManagerUpdateTest) {
            FirmwareManager manager(&EventServer);
            manager.begin(&Client, "http://localhost/images/", "001122334455");

            // check succeeds, update succeeds (but doesn't reboot, obviously)
            HTTPClient::ReturnValue = 200;
            HTTPUpdate::ReturnValue = HTTP_UPDATE_OK;
            manager.tryUpdateFrom("0.1.2");
            Assert::AreEqual(2, InfoListener.getCallCount(), L"info on success");
            Assert::AreEqual(0, ErrorListener.getCallCount(), L"No error on success");
            Assert::AreEqual("Firmware not updated (2/0): OK\n", InfoListener.getPayload(), L"warning message");

            // check fails
            HTTPClient::ReturnValue = 400;
            manager.tryUpdateFrom("0.1.2");
            Assert::AreEqual(2, InfoListener.getCallCount(), L"no info on check failure");
            Assert::AreEqual(1, ErrorListener.getCallCount(), L"Erro on check failure");
            Assert::AreEqual("Firmware version check failed with response code 400\n", ErrorListener.getPayload(),
                             L"Error correct");

            // check succeeds and update fails
            HTTPClient::ReturnValue = 200;
            HTTPUpdate::ReturnValue = HTTP_UPDATE_FAILED;
            manager.tryUpdateFrom("0.1.2");
            Assert::AreEqual(3, InfoListener.getCallCount(), L"new info on update failure");
            Assert::AreEqual("Current firmware version: '0.1.2'; available version: '0.1.1'\n", InfoListener.getPayload(),
                             "info on update failure OK");
            Assert::AreEqual(2, ErrorListener.getCallCount(), L"error on update failure");
            Assert::AreEqual("Firmware update failed (0): OK\n", ErrorListener.getPayload(),
                             L"error message on update failure OK");

            HTTPUpdate::ReturnValue = HTTP_UPDATE_OK;
            // check succeeds but no update needed
            manager.tryUpdateFrom("0.1.1");
            Assert::AreEqual(3, InfoListener.getCallCount(), L"no info on check without update");
            Assert::AreEqual(2, ErrorListener.getCallCount(), L"no error on check without update");

        }

    };

    WiFiClient FirmwareManagerTest::Client;
    EventServer FirmwareManagerTest::EventServer;
    TestEventClient FirmwareManagerTest::InfoListener("info", &EventServer);
    TestEventClient FirmwareManagerTest::ErrorListener("error", &EventServer);
}
