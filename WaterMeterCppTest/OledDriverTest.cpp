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
#include <ESP.h>

#include "TestEventClient.h"
#include "AssertHelper.h"
#include "../WaterMeterCpp/OledDriver.h"
#include "../WaterMeterCpp/EventServer.h"
#include "../WaterMeterCpp/ConnectionState.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {

    TEST_CLASS(OledDriverTest) {

public:
    static EventServer eventServer;

    static void publishConnectionState(ConnectionState connectionState) {
        eventServer.publish(Topic::Connection, static_cast<long>(connectionState));
    }

    TEST_METHOD(oledDriverCycleScriptTest) {
        OledDriver oledDriver(&eventServer);
        TestEventClient client(&eventServer);
        eventServer.subscribe(&client, Topic::NoDisplayFound);
        const auto display = oledDriver.getDriver();
        display->sensorPresent(false);
        Assert::IsFalse(oledDriver.begin(), L"No display");
        Assert::AreEqual(1, client.getCallCount(), L"No-display message sent");
        display->sensorPresent(true);
        client.reset();

        Assert::IsTrue(oledDriver.begin(), L"Display found");
        Assert::AreEqual<int16_t>(0, display->getX(), L"cursor X=0");
        Assert::AreEqual<int16_t>(24, display->getY(), L"cursor Y=24");
        Assert::AreEqual("Starting", display->getMessage(), L"Message OK");
        Assert::AreEqual(0, client.getCallCount(), L"No messages");
        Assert::AreEqual(0u, oledDriver.display(), L"No need to display");

        publishConnectionState(ConnectionState::CheckFirmware);
        Assert::AreEqual(0b11110001, display->getFirstByte(), L"First byte of firmware logo ok");
        Assert::AreEqual<int16_t>(118, display->getX(), L"firmware X=118");
        Assert::AreEqual<int16_t>(0, display->getY(), L"firmware Y=0");
        Assert::AreEqual(25u, oledDriver.display(), L"display was needed");
        Assert::AreEqual(0u, oledDriver.display(), L"No need to display");

        const auto testValue = "23456.7890123";
        eventServer.publish(Topic::Meter, testValue);
        Assert::AreEqual<int16_t>(0, display->getX(), L"Meter X=0");
        Assert::AreEqual<int16_t>(8, display->getY(), L"Meter Y=8");
        Assert::AreEqual("23456.7890123 m3 ", display->getMessage(), L"Meter message OK");

        eventServer.publish(Topic::Alert, LONG_TRUE);
        Assert::AreEqual(0b11111100, display->getFirstByte(), L"First byte of alert logo ok");
        Assert::AreEqual<int16_t>(108, display->getX(), L"alert on X=108");
        Assert::AreEqual<int16_t>(0, display->getY(), L"alert on Y=0");

        publishConnectionState(ConnectionState::WifiReady);
        Assert::AreEqual(0b01111000, display->getFirstByte(), L"First byte of wifi logo ok");
        Assert::AreEqual<int16_t>(118, display->getX(), L"Wifi X=118");
        Assert::AreEqual<int16_t>(0, display->getY(), L"Wifi Y=0");

        eventServer.publish(Topic::Flow, LONG_TRUE);
        Assert::AreEqual(0b00010000, display->getFirstByte(), L"First byte of flow logo ok");
        Assert::AreEqual<int16_t>(98, display->getX(), L"Flow X=98");
        Assert::AreEqual<int16_t>(0, display->getY(), L"Flow Y=0");

        publishConnectionState(ConnectionState::MqttReady);
        Assert::AreEqual(0b00000000, display->getFirstByte(), L"First byte of mqtt logo ok");
        Assert::AreEqual<int16_t>(118, display->getX(), L"mqtt X=118");
        Assert::AreEqual<int16_t>(0, display->getY(), L"mqtt Y=0");

        eventServer.publish(Topic::Alert, LONG_FALSE);
        Assert::AreEqual<int16_t>(108, display->getX(), L"alert off X=108");
        Assert::AreEqual<int16_t>(0, display->getY(), L"alert off Y=0");
        Assert::AreEqual<int16_t>(7, display->getHeight(), L"alert off H=7");
        Assert::AreEqual<int16_t>(8, display->getWidth(), L"alert off W=8");
        Assert::AreEqual<uint16_t>(BLACK, display->getForegroundColor());

        publishConnectionState(ConnectionState::RequestTime);
        Assert::AreEqual(0b00110000, display->getFirstByte(), L"First byte of time logo ok");
        Assert::AreEqual<int16_t>(118, display->getX(), L"Time X=118");
        Assert::AreEqual<int16_t>(0, display->getY(), L"Time Y=0");

        eventServer.publish(Topic::NoSensorFound, LONG_TRUE);
        Assert::AreEqual(0b00010011, display->getFirstByte(), L"First byte of missing sensor logo ok");
        Assert::AreEqual<int16_t>(108, display->getX(), L"missing sensor X=108");
        Assert::AreEqual<int16_t>(0, display->getY(), L"missing sensor Y=0");

        publishConnectionState(ConnectionState::Disconnected);
        Assert::AreEqual<int16_t>(118, display->getX(), L"disconnected X=118");
        Assert::AreEqual<int16_t>(0, display->getY(), L"disconnected Y=0");
        Assert::AreEqual<int16_t>(7, display->getHeight(), L"disconnected H=7");
        Assert::AreEqual<int16_t>(8, display->getWidth(), L"disconnected W=8");
        Assert::AreEqual<uint16_t>(BLACK, display->getForegroundColor(), L"disconnected C=BLACK");

        eventServer.publish(Topic::SensorWasReset, LONG_TRUE);
        Assert::AreEqual(0b00010000, display->getFirstByte(), L"First byte of reset logo ok");
        Assert::AreEqual<int16_t>(108, display->getX(), L"reset X=108");
        Assert::AreEqual<int16_t>(0, display->getY(), L"reset Y=0");

        eventServer.publish(Topic::Peaks, 4321);
        Assert::AreEqual<int16_t>(0, display->getX(), L"Peaks X=0");
        Assert::AreEqual<int16_t>(0, display->getY(), L"Peaks Y=0");
        Assert::AreEqual("Peaks: 4321", display->getMessage(), L"Peaks message OK"); 

        eventServer.publish(Topic::Blocked, LONG_TRUE);
        Assert::AreEqual(0b00111000, display->getFirstByte(), L"First byte of blocked logo ok");
        Assert::AreEqual<int16_t>(108, display->getX(), L"Blocked X=0");
        Assert::AreEqual<int16_t>(0, display->getY(), L"Blocked Y=0");
    }

    };
    EventServer OledDriverTest::eventServer;

}
