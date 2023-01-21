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
#include "../WaterMeterCpp/FirmwareManager.h"
#include "FirmwareManagerDriver.h"

#include "HTTPClient.h"
#include "HTTPUpdate.h"

namespace WaterMeterCppTest {
    
    class FirmwareManagerTest : public testing::Test {
    public:
        static WiFiClient client;
        static EventServer eventServer;
        static TestEventClient infoListener;
        static TestEventClient errorListener;
        static constexpr FirmwareConfig FIRMWARE_CONFIG{"http://localhost/images/"};

        // ReSharper disable once CppInconsistentNaming
        static void SetUpTestCase() {
            eventServer.subscribe(&infoListener, Topic::Info);
            eventServer.subscribe(&errorListener, Topic::ConnectionError);
        }

        void SetUp() override {
            infoListener.reset();
            errorListener.reset();
        }
    };

    WiFiClient FirmwareManagerTest::client;
    EventServer FirmwareManagerTest::eventServer;
    TestEventClient FirmwareManagerTest::infoListener(&eventServer);
    TestEventClient FirmwareManagerTest::errorListener(&eventServer);
    
    TEST_F(FirmwareManagerTest, firmwareManagerCheckSucceedsNoUpdateNeededTest) {
        const WiFiClientFactory wifiClientFactory(nullptr);
        FirmwareManagerDriver manager(&eventServer, &wifiClientFactory, &FIRMWARE_CONFIG, "0.1.1");
        manager.begin("001122334455");
        HTTPClient::ReturnValue = 200;
        HTTPUpdate::ReturnValue = HTTP_UPDATE_OK;
        manager.tryUpdate();
        EXPECT_EQ(1, infoListener.getCallCount()) << "info on check without update";
        EXPECT_STREQ("Already on latest firmware: '0.1.1'", infoListener.getPayload()) << "info on check without update OK";
        EXPECT_EQ(0, errorListener.getCallCount()) << "no error on check without update";
    }


    TEST_F(FirmwareManagerTest, firmwareManagerCheckSucceedsUpdateFailsTest) {
        const WiFiClientFactory wifiClientFactory(nullptr);
        FirmwareManagerDriver manager(&eventServer, &wifiClientFactory, &FIRMWARE_CONFIG, "0.1.2");
        manager.begin("001122334455");
        // check succeeds and update fails
        HTTPClient::ReturnValue = 200;
        HTTPUpdate::ReturnValue = HTTP_UPDATE_FAILED;
        manager.tryUpdate();
        EXPECT_FALSE(manager.updateAvailable()) << "No updating after first update attempt";
        EXPECT_EQ(1, infoListener.getCallCount()) << "new info on update failure";
        EXPECT_STREQ("Current firmware: '0.1.2'; available: '0.1.1'", infoListener.getPayload()) << "info on update failure OK";
        EXPECT_EQ(1, errorListener.getCallCount()) << "error on update failure";
        EXPECT_STREQ("Firmware update failed (0): OK", errorListener.getPayload()) << "error message on update failure OK";
    }

    TEST_F(FirmwareManagerTest, firmwareManagerFailedCheckTest) {
        const WiFiClientFactory wifiClientFactory(nullptr);
        FirmwareManagerDriver manager(&eventServer, &wifiClientFactory, &FIRMWARE_CONFIG, "0.1.1");
        manager.begin("001122334455");

        HTTPClient::ReturnValue = 400;
        EXPECT_FALSE(manager.updateAvailable()) << "No update for version 0.1.1";
        EXPECT_EQ(1, infoListener.getCallCount()) << "info";
        EXPECT_EQ(1, errorListener.getCallCount()) << "Error";
        EXPECT_STREQ("Firmware version check failed with response code 400. URL:", errorListener.getPayload()) << "Error correct";
        EXPECT_STREQ("http://localhost/images/001122334455.version", infoListener.getPayload()) << "Info OK";
    }

    TEST_F(FirmwareManagerTest, firmwareManagerNoUpdateAvailableTest) {
        const WiFiClientFactory wifiClientFactory(nullptr);
        FirmwareManagerDriver manager(&eventServer, &wifiClientFactory, &FIRMWARE_CONFIG, "0.1.1");
        manager.begin("001122334455");

        // Successful check, same version
        HTTPClient::ReturnValue = 200;
        EXPECT_FALSE(manager.updateAvailable()) << "No update for version 0.1.1";
        EXPECT_EQ(1, infoListener.getCallCount()) << "info";
        EXPECT_EQ(0, errorListener.getCallCount()) << "No error";
    }

    TEST_F(FirmwareManagerTest, firmwareManagerOtherVersionTest) {
        const WiFiClientFactory wifiClientFactory(nullptr);
        FirmwareManagerDriver manager(&eventServer, &wifiClientFactory, &FIRMWARE_CONFIG, "0.1.2");
        manager.begin("112233445566");

        HTTPClient::ReturnValue = 200;
        EXPECT_TRUE(manager.updateAvailable()) << "update for version 0.1.2 available";
        EXPECT_EQ(1, infoListener.getCallCount()) << "Info called";
        EXPECT_EQ(0, errorListener.getCallCount()) << "No error";
        EXPECT_STREQ("Current firmware: '0.1.2'; available: '0.1.1'", infoListener.getPayload()) << "Info correct";
    }

    TEST_F(FirmwareManagerTest, firmwareManagerUpdateCheckFailsTest) {
        const WiFiClientFactory wifiClientFactory(nullptr);
        FirmwareManager manager(&eventServer, &wifiClientFactory, &FIRMWARE_CONFIG, "0.1.2");
        manager.begin("001122334455");
        HTTPClient::ReturnValue = 400;
        manager.tryUpdate();
        EXPECT_EQ(1, infoListener.getCallCount()) << "info on check failure";
        EXPECT_EQ(1, errorListener.getCallCount()) << "Erro on check failure";
        EXPECT_STREQ("Firmware version check failed with response code 400. URL:", errorListener.getPayload()) << "Error correct";
        EXPECT_STREQ("http://localhost/images/001122334455.version", infoListener.getPayload()) << "Info OK";

        // second call doesn't do anything
        manager.tryUpdate();
        EXPECT_EQ(1, infoListener.getCallCount()) << "Second call doesn't log info";
        EXPECT_EQ(1, errorListener.getCallCount()) << "Second call doesn't log errors";
    }

    TEST_F(FirmwareManagerTest, firmwareManagerUpdateTest) {
        TestEventClient progressListener(&eventServer);
        eventServer.subscribe(&progressListener, Topic::UpdateProgress);
        const WiFiClientFactory wifiClientFactory(nullptr);
        FirmwareManager manager(&eventServer, &wifiClientFactory, &FIRMWARE_CONFIG, "0.1.2");
        manager.begin("001122334455");

        // check succeeds, update succeeds (but doesn't reboot, obviously)
        HTTPClient::ReturnValue = 200;
        HTTPUpdate::ReturnValue = HTTP_UPDATE_OK;
        manager.tryUpdate();
        EXPECT_EQ(2, infoListener.getCallCount()) << "info on success";
        EXPECT_EQ(0, errorListener.getCallCount()) << "No error on success";
        EXPECT_STREQ("Firmware not updated (2/0): OK", infoListener.getPayload()) << "warning message";
        EXPECT_EQ(2, progressListener.getCallCount()) << "progress called twice (mock httpUpdate)";
        EXPECT_STREQ("100", progressListener.getPayload()) << "progress 100";

        // second call doesn't do anything
        manager.tryUpdate();
        EXPECT_EQ(2, infoListener.getCallCount()) << "Second call doesn't log info";
        EXPECT_EQ(0, errorListener.getCallCount()) << "Second call doesn't log errors";
        EXPECT_EQ(2, progressListener.getCallCount()) << "progress not called again";
    }
}
