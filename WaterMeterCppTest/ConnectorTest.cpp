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

#include "pch.h"

#include <CppUnitTest.h>

#include <ESP.h>
#include "MqttGatewayMock.h"
#include "WiFiMock.h"
#include "../WaterMeterCpp/Connector.h"
// ReSharper disable once CppUnusedIncludeDirective -- false positive
#include "StateHelper.h"
#include "TimeServerMock.h"
#include "../WaterMeterCpp/DataQueue.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
    TEST_CLASS(ConnectorTest) {
    public:
        static Preferences preferences;
        static Configuration configuration;
        static EventServer eventServer;
        static Log logger;
        static WiFiClientFactory wifiClientFactory;
        static WiFiMock wifiMock;
        static MqttGatewayMock mqttGatewayMock;
        static PubSubClient mqttClient;
        static Connector connector;
        static FirmwareManager firmwareManager;
        static TimeServerMock timeServer;
        static PayloadBuilder payloadBuilder;
        static Serializer serializer;
        static QueueHandle_t queueHandle;
        static QueueClient queueClient1;
        static QueueClient queueClient2;
        static DataQueue dataQueue;
        static DataQueue commsDataQueue;

        TEST_CLASS_INITIALIZE(connectorTestInitialize) {
            queueClient1.begin(queueClient2.getQueueHandle());
            queueClient2.begin();
        }

        TEST_METHOD(connectorMaxWifiFailuresTest) {
            connector.setup(&configuration);
            wifiMock.setIsConnected(false);

            Assert::AreEqual(ConnectionState::WifiConnecting, connector.loop(), L"Connecting");
            delay(WIFI_INITIAL_WAIT_DURATION / 1000);

            for (unsigned int i = 0; i < MAX_RECONNECT_FAILURES; i++) {
                Assert::AreEqual(ConnectionState::Disconnected, connector.loop(), L"Disconnected");
                Assert::AreEqual(ConnectionState::WifiConnecting, connector.loop(), L"Connecting 2");
                delay(WIFI_RECONNECT_WAIT_DURATION / 1000);
            }
            Assert::AreEqual(ConnectionState::Init, connector.loop(), L"Failed too many times, re-init");
        }

        TEST_METHOD(connectorReInitTest) {
            connector.setup(&configuration);
            wifiMock.setIsConnected(false);
            wifiMock.setNeedsReconnect(true);
            Assert::AreEqual(ConnectionState::WifiConnecting, connector.connect(), L"Connecting");
            wifiMock.setIsConnected(true);

            Assert::AreEqual(ConnectionState::WifiConnected, connector.connect(), L"Connected");
            Assert::AreEqual(ConnectionState::Init, connector.connect(), L"Back to init");
            wifiMock.setNeedsReconnect(false);
            Assert::AreEqual(ConnectionState::WifiConnecting, connector.connect(), L"Connecting");
            Assert::AreEqual(ConnectionState::WifiConnected, connector.connect(), L"Connected");
            wifiMock.setIsConnected(false);
            Assert::AreEqual(ConnectionState::Disconnected, connector.connect(), L"Disconnected");
        }

        TEST_METHOD(connectorScriptTest) {
            timeServer.reset();
            // connect before timeout
            connector.setup(&configuration);
            wifiMock.setNeedsReconnect(false);
            wifiMock.setIsConnected(false);
            Assert::AreEqual(ConnectionState::WifiConnecting, connector.connect(), L"Wifi connecting");
            Assert::AreEqual(ConnectionState::WifiConnecting, connector.connect(), L"Wifi connecting 2");
            delay(3000);
            Assert::AreEqual(ConnectionState::WifiConnecting, connector.connect(), L"Wifi connecting 3");
            wifiMock.setIsConnected(true);
            Assert::AreEqual(ConnectionState::WifiConnected, connector.connect(), L"Wifi connected");
            Assert::AreEqual(ConnectionState::RequestTime, connector.connect(), L"Set time");
            Assert::AreEqual(ConnectionState::SettingTime, connector.connect(), L"Setting time");
            Assert::AreEqual(ConnectionState::CheckFirmware, connector.connect(), L"Checking firmware");
            Assert::AreEqual(ConnectionState::WifiReady, connector.connect(), L"Wifi ready");

            // happy path for mqtt
            mqttGatewayMock.setIsConnected(true);
            Assert::AreEqual(ConnectionState::MqttConnecting, connector.connect(), L"Connecting to MQTT 1");
            Assert::AreEqual(ConnectionState::MqttConnected, connector.connect(), L"Connected to MQTT 1");
            const auto timestampReady = micros();
            Assert::AreEqual(ConnectionState::MqttReady, connector.loop(), L"MQTT ready");

            // we're doing the division to filter out the few micros that the statement costs
            Assert::AreEqual(50UL, (micros() - timestampReady) / 1000UL, L"50 ms wait time when ready");
            Assert::AreEqual(
                ConnectionState::MqttReady, connector.connect(),
                L"MQTT stays ready if nothing changges");

            // disconnecting Wifi should change state to Disconnected
            wifiMock.setIsConnected(false);
            Assert::AreEqual(ConnectionState::Disconnected, connector.loop(), L"Disconnects if wifi is down");

            Assert::AreEqual(ConnectionState::WifiConnecting, connector.connect(), L"Reconnecting Wifi");
            delay(20000);
            Assert::AreEqual(ConnectionState::Disconnected, connector.connect(), L"Still down, disconnect");
            wifiMock.setIsConnected(true);
            Assert::AreEqual(
                ConnectionState::WifiConnecting, connector.connect(),
                L"Connecting after Wifi comes up");
            Assert::AreEqual(ConnectionState::WifiConnected, connector.connect(), L"Wifi reconnected");
            Assert::AreEqual(ConnectionState::RequestTime, connector.connect(), L"SetTime 2(does nothing)");
            Assert::AreEqual(ConnectionState::CheckFirmware, connector.connect(), L"Checking firmware 2 (does nothing)");

            Assert::AreEqual(ConnectionState::WifiReady, connector.connect(), L"Wifi Ready 2");
            Assert::AreEqual(ConnectionState::MqttConnecting, connector.connect(), L"Connecting to MQTT 2");
            Assert::AreEqual(ConnectionState::MqttConnected, connector.connect(), L"Connected to MQTT 2");
            Assert::AreEqual(ConnectionState::MqttReady, connector.connect(), L"Mqtt Ready 2");

            // disconnecting MQTT should put the state to Wifi connected
            mqttGatewayMock.setIsConnected(false);
            Assert::AreEqual(ConnectionState::WifiReady, connector.connect(), L"back to Wifi Ready");
            Assert::AreEqual(ConnectionState::MqttConnecting, connector.connect(), L"Connecting to MQTT 3");
            Assert::AreEqual(ConnectionState::WaitingForMqttReconnect, connector.connect(), L"awaiting MQTT timeout");
            delay(2000);
            Assert::AreEqual(ConnectionState::WifiReady, connector.connect(), L"done waiting, back to Wifi Ready");
            wifiMock.setIsConnected(false);
            Assert::AreEqual(ConnectionState::Disconnected, connector.connect(), L"Wifi disconnected too");
            wifiMock.setIsConnected(true);
            mqttGatewayMock.setIsConnected(true);
            Assert::AreEqual(ConnectionState::WifiConnecting, connector.connect(), L"Connecting Wifi 4");
            Assert::AreEqual(ConnectionState::WifiConnected, connector.connect(), L"Wifi Connected 4");
            Assert::AreEqual(ConnectionState::RequestTime, connector.connect(), L"SetTime 3(does nothing)");
            Assert::AreEqual(ConnectionState::CheckFirmware, connector.connect(), L"Checking firmware 3 (does nothing)");

            Assert::AreEqual(ConnectionState::WifiReady, connector.connect(), L"Wifi Ready 4");
            Assert::AreEqual(ConnectionState::MqttConnecting, connector.connect(), L"Connecting to MQTT 4");
            Assert::AreEqual(ConnectionState::MqttConnected, connector.connect(), L"Connected to MQTT 4");
            mqttGatewayMock.setIsConnected(false);
            Assert::AreEqual(ConnectionState::WifiReady, connector.connect(), L"announce failed, to Wifi ready");
            mqttGatewayMock.setIsConnected(true);
            Assert::AreEqual(ConnectionState::MqttConnecting, connector.connect(), L"Connecting to MQTT 5");
            Assert::AreEqual(ConnectionState::MqttConnected, connector.connect(), L"Connected to MQTT 5");
            Assert::AreEqual(ConnectionState::MqttReady, connector.connect(), L"Final MQTT Ready");
        }

        TEST_METHOD(connectorTimeFailTest) {
            connector.setup(&configuration);
            wifiMock.setIsConnected(true);
            wifiMock.setNeedsReconnect(false);

            Assert::AreEqual(ConnectionState::WifiConnecting, connector.connect(), L"Connecting");
            Assert::AreEqual(ConnectionState::WifiConnected, connector.connect(), L"Connected");
            Assert::AreEqual(ConnectionState::RequestTime, connector.connect(), L"Request time 1");
            // make sure time was not set
            timeServer.reset();
            Assert::AreEqual(ConnectionState::SettingTime, connector.connect(), L"Waiting for time to be set 1");
            // and again, since the standard mock mechanism switches it on
            timeServer.reset();
            Assert::AreEqual(ConnectionState::SettingTime, connector.connect(), L"Waiting for time to be set 2");
            delay(10000);
            Assert::AreEqual(ConnectionState::WifiConnected, connector.connect(), L"Back to Connected");
            Assert::AreEqual(ConnectionState::RequestTime, connector.connect(), L"Request time 2");
            // now let timeServer succeed
            timeServer.setTime();
            Assert::AreEqual(ConnectionState::CheckFirmware, connector.connect(), L"Time was set");
        }

        TEST_METHOD(connectorWifiInitTest) {
            connector.setup(&configuration);

            wifiMock.setNeedsReconnect(true);
            wifiMock.setIsConnected(true);
            timeServer.reset();

            Assert::AreEqual(ConnectionState::WifiConnecting, connector.connect(), L"Connecting");
            Assert::AreEqual(ConnectionState::WifiConnected, connector.connect(), L"Connected");
            Assert::AreEqual(ConnectionState::Init, connector.connect(), L"Disconnected (reconnect)");
            wifiMock.setNeedsReconnect(false);
            Assert::AreEqual(ConnectionState::WifiConnecting, connector.connect(), L"Reconnecting");
            Assert::AreEqual(ConnectionState::WifiConnected, connector.connect(), L"Reconnected");
            Assert::AreEqual(ConnectionState::RequestTime, connector.connect(), L"Requesting time");
            // disconnected just after time was requested. Time still comes in
            wifiMock.setIsConnected(false);
            Assert::AreEqual(ConnectionState::SettingTime, connector.connect(), L"Setting time");

            Assert::AreEqual(ConnectionState::Disconnected, connector.connect(), L"Disconnected");
            Assert::AreEqual(ConnectionState::WifiConnecting, connector.connect(), L"Connecting");
            Assert::AreEqual(ConnectionState::WifiConnecting, connector.connect(), L"Still Connecting");
            wifiMock.setIsConnected(true);
            Assert::AreEqual(ConnectionState::WifiConnected, connector.connect(), L"Connected 2");
            Assert::AreEqual(ConnectionState::RequestTime, connector.connect(), L"Requesting time 2");
            Assert::AreEqual(ConnectionState::CheckFirmware, connector.connect(), L"Checking firmware");
        }
    };

    Preferences ConnectorTest::preferences;
    Configuration ConnectorTest::configuration(&preferences);
    EventServer ConnectorTest::eventServer;
    Log ConnectorTest::logger(&eventServer, nullptr);
    WiFiClientFactory ConnectorTest::wifiClientFactory(nullptr);
    WiFiMock ConnectorTest::wifiMock(&eventServer, nullptr);
    PubSubClient ConnectorTest::mqttClient;
    MqttGatewayMock ConnectorTest::mqttGatewayMock(&eventServer, &mqttClient, &wifiClientFactory);
    TimeServerMock ConnectorTest::timeServer;
    FirmwareConfig firmwareConfig{ "http://localhost/" };
    FirmwareManager ConnectorTest::firmwareManager(&eventServer, &wifiClientFactory, &firmwareConfig, "0.99.3");
    PayloadBuilder ConnectorTest::payloadBuilder;
    Serializer ConnectorTest::serializer(&eventServer, &payloadBuilder);
    DataQueuePayload payload;
    DataQueue ConnectorTest::dataQueue(&eventServer, &payload);
    DataQueue ConnectorTest::commsDataQueue(&eventServer, &payload, 1, 2, 1, 2);

    QueueHandle_t ConnectorTest::queueHandle = nullptr;
    QueueClient ConnectorTest::queueClient1(&eventServer, &logger, 10);
    QueueClient ConnectorTest::queueClient2(&eventServer, &logger, 10);
    Connector ConnectorTest::connector(&eventServer, &wifiMock, &mqttGatewayMock, &timeServer, &firmwareManager, &dataQueue,
                                       &commsDataQueue, &serializer, &queueClient1, &queueClient2);
}
