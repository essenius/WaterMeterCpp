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

#include <CppUnitTest.h>

#include "MqttGatewayMock.h"
#include "WifiMock.h"
#include "../WaterMeterCpp/Connection.h"
#include "../WaterMeterCpp/ArduinoMock.h"
#include "StateHelper.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
    TEST_CLASS(ConnectionTest) {
    public:
        static EventServer EventServer;
        static WifiMock WifiMock;
        static MqttGatewayMock MqttGatewayMock;
        static Connection Connection;

        TEST_METHOD(connectionWifiInitTest) {
            Connection.begin();
            WifiMock.setNeedsReconnect(true);
            WifiMock.setIsConnected(true);
            Assert::AreEqual(ConnectionState::WifiConnecting, Connection.connectWifi(), L"Connecting");
            Assert::AreEqual(ConnectionState::WifiConnected, Connection.connectWifi(), L"Connected");
            Assert::AreEqual(ConnectionState::Init, Connection.connectWifi(), L"Disconnected (reconnect)");
            WifiMock.setNeedsReconnect(false);
            Assert::AreEqual(ConnectionState::WifiConnecting, Connection.connectWifi(), L"Reconnecting");
            Assert::AreEqual(ConnectionState::WifiConnected, Connection.connectWifi(), L"Reconnected");
            Assert::AreEqual(ConnectionState::WifiReady, Connection.connectWifi(), L"Ready");
            Assert::AreEqual(ConnectionState::WifiReady, Connection.connectWifi(), L"Still ready");
            WifiMock.setIsConnected(false);
            Assert::AreEqual(ConnectionState::Disconnected, Connection.connectWifi(), L"Disconnected");
            Assert::AreEqual(ConnectionState::WifiConnecting, Connection.connectWifi(), L"Connecting");
            Assert::AreEqual(ConnectionState::WifiConnecting, Connection.connectWifi(), L"Still Connecting");
            WifiMock.setIsConnected(true);
            Assert::AreEqual(ConnectionState::WifiConnected, Connection.connectWifi(), L"Connected 2");
            Assert::AreEqual(ConnectionState::WifiReady, Connection.connectWifi(), L"Ready 2");
        }

        TEST_METHOD(connectionMaxWifiFailuresTest) {
            Connection.begin();
            WifiMock.setIsConnected(false);

            Assert::AreEqual(ConnectionState::WifiConnecting, Connection.connectMqtt(), L"Connecting");
            delay(WIFI_INITIAL_WAIT_DURATION / 1000);

            for (int i = 0; i < MAX_RECONNECT_FAILURES; i++) {
                Assert::AreEqual(ConnectionState::Disconnected, Connection.connectMqtt(), L"Disconnected");
                Assert::AreEqual(ConnectionState::WifiConnecting, Connection.connectMqtt(), L"Connecting 2");
                delay(WIFI_RECONNECT_WAIT_DURATION / 1000);
            }
            Assert::AreEqual(ConnectionState::Init, Connection.connectMqtt(), L"Failed too many times, re-init");
        }

        TEST_METHOD(connectionScriptTest) {
            // connect before timeout
            Connection.begin();
            WifiMock.setNeedsReconnect(false);
            WifiMock.setIsConnected(false);
            Assert::AreEqual(ConnectionState::WifiConnecting, Connection.connectMqtt(), L"Wifi connecting");
            Assert::AreEqual(ConnectionState::WifiConnecting, Connection.connectMqtt(), L"Wifi connecting 2");
            delay(3000);
            Assert::AreEqual(ConnectionState::WifiConnecting, Connection.connectMqtt(), L"Wifi connecting 3");
            WifiMock.setIsConnected(true);
            Assert::AreEqual(ConnectionState::WifiConnected, Connection.connectMqtt(), L"Wifi connected");
            Assert::AreEqual(ConnectionState::WifiReady, Connection.connectMqtt(), L"Wifi ready");
            // happy path for mqtt
            MqttGatewayMock.setIsConnected(true);
            Assert::AreEqual(ConnectionState::MqttConnecting, Connection.connectMqtt(), L"Connecting to MQTT");
            Assert::AreEqual(ConnectionState::MqttConnected, Connection.connectMqtt(), L"Connected to MQTT 1");
            Assert::AreEqual(ConnectionState::MqttConnected, Connection.connectMqtt(), L"Connected to MQTT 2");
            Assert::AreEqual(ConnectionState::MqttConnected, Connection.connectMqtt(), L"Connected to MQTT 3");
            Assert::AreEqual(ConnectionState::MqttReady, Connection.connectMqtt(), L"MQTT ready");
            Assert::AreEqual(ConnectionState::MqttReady, Connection.connectMqtt(),
                             L"MQTT stays ready if nothing changges");
            // disconnecting Wifi should change state to Disconnected
            WifiMock.setIsConnected(false);
            Assert::AreEqual(ConnectionState::Disconnected, Connection.connectMqtt(), L"Disconnects if wifi is down");
            Assert::AreEqual(ConnectionState::WifiConnecting, Connection.connectMqtt(), L"Reconnecting Wifi");
            delay(20000);
            Assert::AreEqual(ConnectionState::Disconnected, Connection.connectMqtt(), L"Still down, disconnect");
            WifiMock.setIsConnected(true);
            Assert::AreEqual(ConnectionState::WifiConnecting, Connection.connectMqtt(),
                             L"Connecting after Wifi comes up");
            Assert::AreEqual(ConnectionState::WifiConnected, Connection.connectMqtt(), L"Wifi reonnected");
            Assert::AreEqual(ConnectionState::WifiReady, Connection.connectMqtt(), L"Wifi Ready 2");
            Assert::AreEqual(ConnectionState::MqttConnecting, Connection.connectMqtt(), L"Connecting to MQTT 2");
            Assert::AreEqual(ConnectionState::MqttConnected, Connection.connectMqtt(), L"Connected to MQTT 4");
            Assert::AreEqual(ConnectionState::MqttConnected, Connection.connectMqtt(), L"Connected to MQTT 5");
            Assert::AreEqual(ConnectionState::MqttConnected, Connection.connectMqtt(), L"Connected to MQTT 6");
            Assert::AreEqual(ConnectionState::MqttReady, Connection.connectMqtt(), L"Mqtt Ready 2");

            // disconnecting MQTT should put the state to Wifi connected
            MqttGatewayMock.setIsConnected(false);
            Assert::AreEqual(ConnectionState::WifiReady, Connection.connectMqtt(), L"back to Wifi Ready");
            Assert::AreEqual(ConnectionState::MqttConnecting, Connection.connectMqtt(), L"Connecting to MQTT 3");
            Assert::AreEqual(ConnectionState::MqttConnecting, Connection.connectMqtt(), L"awaiting MQTT timeout");
            delay(2000);
            Assert::AreEqual(ConnectionState::WifiReady, Connection.connectMqtt(), L"failed, back to Wifi Ready");
            WifiMock.setIsConnected(false);
            Assert::AreEqual(ConnectionState::Disconnected, Connection.connectMqtt(), L"Wifi disconnected too");
            WifiMock.setIsConnected(true);
            MqttGatewayMock.setIsConnected(true);
            Assert::AreEqual(ConnectionState::WifiConnecting, Connection.connectMqtt(), L"Connecting Wifi 4");
            Assert::AreEqual(ConnectionState::WifiConnected, Connection.connectMqtt(), L"Wifi Connected 4");
            Assert::AreEqual(ConnectionState::WifiReady, Connection.connectMqtt(), L"Wifi Ready 4");
            Assert::AreEqual(ConnectionState::MqttConnecting, Connection.connectMqtt(), L"Connecting to MQTT 5");
            Assert::AreEqual(ConnectionState::MqttConnected, Connection.connectMqtt(), L"Connected to MQTT 4");
            MqttGatewayMock.setIsConnected(false);
            Assert::AreEqual(ConnectionState::WifiReady, Connection.connectMqtt(), L"announce failed, to Wifi ready");
            MqttGatewayMock.setIsConnected(true);
            Assert::AreEqual(ConnectionState::MqttConnecting, Connection.connectMqtt(), L"Connecting to MQTT 6");
            Assert::AreEqual(ConnectionState::MqttConnected, Connection.connectMqtt(), L"Connected to MQTT 7");
            Assert::AreEqual(ConnectionState::MqttConnected, Connection.connectMqtt(), L"Connected to MQTT 8");
            Assert::AreEqual(ConnectionState::MqttConnected, Connection.connectMqtt(), L"Connected to MQTT 9");
            Assert::AreEqual(ConnectionState::MqttReady, Connection.connectMqtt(), L"Final MQTT Ready");
        }
    };

    EventServer ConnectionTest::EventServer;
    WifiMock ConnectionTest::WifiMock(&EventServer);
    MqttGatewayMock ConnectionTest::MqttGatewayMock(&EventServer);
    Connection ConnectionTest::Connection(&EventServer, &WifiMock, &MqttGatewayMock);

}
