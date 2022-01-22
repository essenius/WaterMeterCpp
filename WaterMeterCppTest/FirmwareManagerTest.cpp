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
        static WiFiClient client;
        static EventServer eventServer;
        static TestEventClient infoListener;
        static TestEventClient errorListener;

        TEST_CLASS_INITIALIZE(firmwareClassInitialize) {
            eventServer.subscribe(&infoListener, Topic::Info);
            eventServer.subscribe(&errorListener, Topic::Error);
        }

        TEST_METHOD_INITIALIZE(firmwareMethodInitialize) {
            infoListener.reset();
            errorListener.reset();
        }

        TEST_METHOD(firmwareManagerNoUpdateAvailableTest) {
            FirmwareManager manager(&eventServer, "http://localhost/images", "0.1.1");
            manager.begin(&client, "001122334455");

            // Successful check, same version
            HTTPClient::ReturnValue = 200;
            Assert::IsFalse(manager.updateAvailable(), L"No update for version 0.1.1");
            Assert::AreEqual(0, infoListener.getCallCount(), L"No info");
            Assert::AreEqual(0, errorListener.getCallCount(), L"No error");
        }

        TEST_METHOD(firmwareManagerFailedCheckTest) {
            FirmwareManager manager(&eventServer, "http://localhost/images", "0.1.1");
            manager.begin(&client, "001122334455");

            HTTPClient::ReturnValue = 400;
            Assert::IsFalse(manager.updateAvailable(), L"No update for version 0.1.1");
            Assert::AreEqual(0, infoListener.getCallCount(), L"No info");
            Assert::AreEqual(1, errorListener.getCallCount(), L"Error");
            Assert::AreEqual("Firmware version check to 'http://localhost/images001122334455.version' failed with response code 400\n", errorListener.getPayload(),
                L"Error correct");
        }

        TEST_METHOD(firmwareManagerOtherVersionTest) {
            FirmwareManager manager(&eventServer, "http://localhost/images", "0.1.2");
            manager.begin(&client, "112233445566");

            HTTPClient::ReturnValue = 200;
            Assert::IsTrue(manager.updateAvailable(), L"update for version 0.1.2 available");
            Assert::AreEqual(1, infoListener.getCallCount(), L"Info called");
            Assert::AreEqual(0, errorListener.getCallCount(), L"No error");
            Assert::AreEqual("Current firmware version: '0.1.2'; available version: '0.1.1'\n", infoListener.getPayload(),
                             L"Info correct");
        }

        TEST_METHOD(firmwareManagerUpdateTest) {
            FirmwareManager manager(&eventServer, "http://localhost/images/", "0.1.2");
            manager.begin(&client, "001122334455");

            // check succeeds, update succeeds (but doesn't reboot, obviously)
            HTTPClient::ReturnValue = 200;
            HTTPUpdate::ReturnValue = HTTP_UPDATE_OK;
            manager.tryUpdate();
            Assert::AreEqual(2, infoListener.getCallCount(), L"info on success");
            Assert::AreEqual(0, errorListener.getCallCount(), L"No error on success");
            Assert::AreEqual("Firmware not updated (2/0): OK\n", infoListener.getPayload(), L"warning message");

            // second call doesn't do anything
            manager.tryUpdate();
            Assert::AreEqual(2, infoListener.getCallCount(), L"Second call doesn't log info");
            Assert::AreEqual(0, errorListener.getCallCount(), L"Second call doesn't log errors");
        }

        TEST_METHOD(FirmwareManagerUpdateCheckFailsTest) {
            FirmwareManager manager(&eventServer, "http://localhost/images/", "0.1.2");
            manager.begin(&client, "001122334455");
            HTTPClient::ReturnValue = 400;
            manager.tryUpdate();
            Assert::AreEqual(0, infoListener.getCallCount(), L"no info on check failure");
            Assert::AreEqual(1, errorListener.getCallCount(), L"Erro on check failure");
            Assert::AreEqual("Firmware version check to 'http://localhost/images/001122334455.version' failed with response code 400\n", errorListener.getPayload(),
                L"Error correct");

            // second call doesn't do anything
            manager.tryUpdate();
            Assert::AreEqual(0, infoListener.getCallCount(), L"Second call doesn't log info");
            Assert::AreEqual(1, errorListener.getCallCount(), L"Second call doesn't log errors");
        }

        TEST_METHOD(firmwareManagerCheckSucceedsUpdateFailsTest) {
            FirmwareManager manager(&eventServer, "http://localhost/images/", "0.1.2");
            manager.begin(&client, "001122334455");
            // check succeeds and update fails
            HTTPClient::ReturnValue = 200;
            HTTPUpdate::ReturnValue = HTTP_UPDATE_FAILED;
            manager.tryUpdate();
            Assert::AreEqual(1, infoListener.getCallCount(), L"new info on update failure");
            Assert::AreEqual("Current firmware version: '0.1.2'; available version: '0.1.1'\n", infoListener.getPayload(),
                "info on update failure OK");
            Assert::AreEqual(1, errorListener.getCallCount(), L"error on update failure");
            Assert::AreEqual("Firmware update failed (0): OK\n", errorListener.getPayload(),
                L"error message on update failure OK");
        }

        TEST_METHOD(firmwareManagerCheckSucceedsNoUpdateNeededTest) {
            FirmwareManager manager(&eventServer, "http://localhost/images/", "0.1.1");
            manager.begin(&client, "001122334455");
            HTTPClient::ReturnValue = 200;
            HTTPUpdate::ReturnValue = HTTP_UPDATE_OK;
            manager.tryUpdate();
            Assert::AreEqual(0, infoListener.getCallCount(), L"no info on check without update");
            Assert::AreEqual(0, errorListener.getCallCount(), L"no error on check without update");
        }
    };

    WiFiClient FirmwareManagerTest::client;
    EventServer FirmwareManagerTest::eventServer;
    TestEventClient FirmwareManagerTest::infoListener(&eventServer);
    TestEventClient FirmwareManagerTest::errorListener(&eventServer);
}
