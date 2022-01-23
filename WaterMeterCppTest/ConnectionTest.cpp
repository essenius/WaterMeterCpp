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
#include "../WaterMeterCpp/Connector.h"
#include "../WaterMeterCpp/ArduinoMock.h"
#include "StateHelper.h"
#include "TimeServerMock.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
    TEST_CLASS(ConnectionTest) {
    public:
        static EventServer eventServer;
        static WifiMock wifiMock;
        static MqttGatewayMock mqttGatewayMock;
        static PubSubClient mqttClient;
        static Connector connection;
        static FirmwareManager firmwareManager;
        static TimeServerMock timeServer;
        static PayloadBuilder payloadBuilder;
        static Serializer serializer;
        static QueueHandle_t queueHandle ;
        static QueueClient queueClient;
        static Log logger;
        static LedDriver ledDriver;
        static DataQueue dataQueue;

        TEST_CLASS_INITIALIZE(connectionTestInitialize) {
            queueClient.begin(queueClient.getQueueHandle());
        }
         TEST_METHOD(connectionWifiInitTest) {
            connection.setup();

            wifiMock.setNeedsReconnect(true);
            wifiMock.setIsConnected(true);
            timeServer.reset();

            Assert::AreEqual(ConnectionState::WifiConnecting, connection.connect(), L"Connecting");
            Assert::AreEqual(ConnectionState::WifiConnected, connection.connect(), L"Connected");
            Assert::AreEqual(ConnectionState::Init, connection.connect(), L"Disconnected (reconnect)");
            wifiMock.setNeedsReconnect(false);
            Assert::AreEqual(ConnectionState::WifiConnecting, connection.connect(), L"Reconnecting");
            Assert::AreEqual(ConnectionState::WifiConnected, connection.connect(), L"Reconnected");
            Assert::AreEqual(ConnectionState::RequestTime, connection.connect(), L"Requesting time");
            // disconnected just after time was requested. Time still comes in
            wifiMock.setIsConnected(false);
            Assert::AreEqual(ConnectionState::SettingTime, connection.connect(), L"Setting time");

            Assert::AreEqual(ConnectionState::Disconnected, connection.connect(), L"Disconnected");
            Assert::AreEqual(ConnectionState::WifiConnecting, connection.connect(), L"Connecting");
            Assert::AreEqual(ConnectionState::WifiConnecting, connection.connect(), L"Still Connecting");
            wifiMock.setIsConnected(true);
            Assert::AreEqual(ConnectionState::WifiConnected, connection.connect(), L"Connected 2");
            Assert::AreEqual(ConnectionState::RequestTime, connection.connect(), L"Requesting time 2");
            Assert::AreEqual(ConnectionState::CheckFirmware, connection.connect(), L"Checking firmware");
         }

        TEST_METHOD(connectionMaxWifiFailuresTest) {
            connection.setup();
            wifiMock.setIsConnected(false);

            Assert::AreEqual(ConnectionState::WifiConnecting, connection.loop(), L"Connecting");
            delay(WIFI_INITIAL_WAIT_DURATION / 1000);

            for (unsigned int i = 0; i < MAX_RECONNECT_FAILURES; i++) {
                Assert::AreEqual(ConnectionState::Disconnected, connection.loop(), L"Disconnected");
                Assert::AreEqual(ConnectionState::WifiConnecting, connection.loop(), L"Connecting 2");
                delay(WIFI_RECONNECT_WAIT_DURATION / 1000);
            }
            Assert::AreEqual(ConnectionState::Init, connection.loop(), L"Failed too many times, re-init");
        }

        TEST_METHOD(connectionReInitTest) {
            connection.setup();
            wifiMock.setIsConnected(false);
            wifiMock.setNeedsReconnect(true);
            Assert::AreEqual(ConnectionState::WifiConnecting, connection.connect(), L"Connecting");
            wifiMock.setIsConnected(true);

            Assert::AreEqual(ConnectionState::WifiConnected, connection.connect(), L"Connected");
            Assert::AreEqual(ConnectionState::Init, connection.connect(), L"Back to init");
            wifiMock.setNeedsReconnect(false);
            Assert::AreEqual(ConnectionState::WifiConnecting, connection.connect(), L"Connecting");
            Assert::AreEqual(ConnectionState::WifiConnected, connection.connect(), L"Connected");
            wifiMock.setIsConnected(false);
            Assert::AreEqual(ConnectionState::Disconnected, connection.connect(), L"Disconnected");
        }

        TEST_METHOD(connectionTimeFailTest) {
            connection.setup();
            wifiMock.setIsConnected(true);
            wifiMock.setNeedsReconnect(false);

            Assert::AreEqual(ConnectionState::WifiConnecting, connection.connect(), L"Connecting");
            Assert::AreEqual(ConnectionState::WifiConnected, connection.connect(), L"Connected");
            Assert::AreEqual(ConnectionState::RequestTime, connection.connect(), L"Request time 1");
            // make sure time was not set
            timeServer.reset();
            Assert::AreEqual(ConnectionState::SettingTime, connection.connect(), L"Waiting for time to be set 1");
            // and again, since the standard mock mechanism switches it on
            timeServer.reset();
            Assert::AreEqual(ConnectionState::SettingTime, connection.connect(), L"Waiting for time to be set 2");
            delay(10000);
            Assert::AreEqual(ConnectionState::WifiConnected, connection.connect(), L"Back to Connected");
            Assert::AreEqual(ConnectionState::RequestTime, connection.connect(), L"Request time 2");
            // now let timeServer succeed
            timeServer.setTime();
            Assert::AreEqual(ConnectionState::CheckFirmware, connection.connect(), L"Time was set");
        }


        TEST_METHOD(connectionScriptTest) {
            // connect before timeout
            connection.setup();
            wifiMock.setNeedsReconnect(false);
            wifiMock.setIsConnected(false);
            Assert::AreEqual(ConnectionState::WifiConnecting, connection.connect(), L"Wifi connecting");
            Assert::AreEqual(ConnectionState::WifiConnecting, connection.connect(), L"Wifi connecting 2");
            delay(3000);
            Assert::AreEqual(ConnectionState::WifiConnecting, connection.connect(), L"Wifi connecting 3");
            wifiMock.setIsConnected(true);
            Assert::AreEqual(ConnectionState::WifiConnected, connection.connect(), L"Wifi connected");
            Assert::AreEqual(ConnectionState::RequestTime, connection.connect(), L"Set time");
            Assert::AreEqual(ConnectionState::SettingTime, connection.connect(), L"Setting time");
            Assert::AreEqual(ConnectionState::CheckFirmware, connection.connect(), L"Checking firmware");

            Assert::AreEqual(ConnectionState::WifiReady, connection.connect(), L"Wifi ready");
            // happy path for mqtt
            mqttGatewayMock.setIsConnected(true);
            Assert::AreEqual(ConnectionState::MqttConnecting, connection.connect(), L"Connecting to MQTT");
            Assert::AreEqual(ConnectionState::MqttConnected, connection.connect(), L"Connected to MQTT 1");
            Assert::AreEqual(ConnectionState::MqttConnected, connection.connect(), L"Connected to MQTT 2");
            Assert::AreEqual(ConnectionState::MqttConnected, connection.connect(), L"Connected to MQTT 3");
            const auto timestampReady = micros();
            Assert::AreEqual(ConnectionState::MqttReady, connection.loop(), L"MQTT ready");
            // we're doing the division to filter out the few micros that the statement costs
            Assert::AreEqual(100UL, (micros() - timestampReady) / 1000UL, L"100 ms wait time when ready");
            Assert::AreEqual(ConnectionState::MqttReady, connection.connect(),
                             L"MQTT stays ready if nothing changges");
            // disconnecting Wifi should change state to Disconnected
            wifiMock.setIsConnected(false);
            const auto timestampDisconnected = micros();
            Assert::AreEqual(ConnectionState::Disconnected, connection.loop(), L"Disconnects if wifi is down");
            Assert::AreEqual(100UL, (micros() - timestampDisconnected) / 1000UL, L"100 ms wait time when not ready (changed - can remove this test)");

            Assert::AreEqual(ConnectionState::WifiConnecting, connection.connect(), L"Reconnecting Wifi");
            delay(20000);
            Assert::AreEqual(ConnectionState::Disconnected, connection.connect(), L"Still down, disconnect");
            wifiMock.setIsConnected(true);
            Assert::AreEqual(ConnectionState::WifiConnecting, connection.connect(),
                             L"Connecting after Wifi comes up");
            Assert::AreEqual(ConnectionState::WifiConnected, connection.connect(), L"Wifi reconnected");
            Assert::AreEqual(ConnectionState::RequestTime, connection.connect(), L"SetTime 2(does nothing)");
            Assert::AreEqual(ConnectionState::CheckFirmware, connection.connect(), L"Checking firmware 2 (does nothing)");

            Assert::AreEqual(ConnectionState::WifiReady, connection.connect(), L"Wifi Ready 2");
            Assert::AreEqual(ConnectionState::MqttConnecting, connection.connect(), L"Connecting to MQTT 2");
            Assert::AreEqual(ConnectionState::MqttConnected, connection.connect(), L"Connected to MQTT 4");
            Assert::AreEqual(ConnectionState::MqttConnected, connection.connect(), L"Connected to MQTT 5");
            Assert::AreEqual(ConnectionState::MqttConnected, connection.connect(), L"Connected to MQTT 6");
            Assert::AreEqual(ConnectionState::MqttReady, connection.connect(), L"Mqtt Ready 2");

            // disconnecting MQTT should put the state to Wifi connected
            mqttGatewayMock.setIsConnected(false);
            Assert::AreEqual(ConnectionState::WifiReady, connection.connect(), L"back to Wifi Ready");
            Assert::AreEqual(ConnectionState::MqttConnecting, connection.connect(), L"Connecting to MQTT 3");
            Assert::AreEqual(ConnectionState::WaitingForMqttReconnect, connection.connect(), L"awaiting MQTT timeout");
            delay(2000);
            Assert::AreEqual(ConnectionState::WifiReady, connection.connect(), L"done waiting, back to Wifi Ready");
            wifiMock.setIsConnected(false);
            Assert::AreEqual(ConnectionState::Disconnected, connection.connect(), L"Wifi disconnected too");
            wifiMock.setIsConnected(true);
            mqttGatewayMock.setIsConnected(true);
            Assert::AreEqual(ConnectionState::WifiConnecting, connection.connect(), L"Connecting Wifi 4");
            Assert::AreEqual(ConnectionState::WifiConnected, connection.connect(), L"Wifi Connected 4");
            Assert::AreEqual(ConnectionState::RequestTime, connection.connect(), L"SetTime 3(does nothing)");
            Assert::AreEqual(ConnectionState::CheckFirmware, connection.connect(), L"Checking firmware 3 (does nothing)");

            Assert::AreEqual(ConnectionState::WifiReady, connection.connect(), L"Wifi Ready 4");
            Assert::AreEqual(ConnectionState::MqttConnecting, connection.connect(), L"Connecting to MQTT 5");
            Assert::AreEqual(ConnectionState::MqttConnected, connection.connect(), L"Connected to MQTT 4");
            mqttGatewayMock.setIsConnected(false);
            Assert::AreEqual(ConnectionState::WifiReady, connection.connect(), L"announce failed, to Wifi ready");
            mqttGatewayMock.setIsConnected(true);
            Assert::AreEqual(ConnectionState::MqttConnecting, connection.connect(), L"Connecting to MQTT 6");
            Assert::AreEqual(ConnectionState::MqttConnected, connection.connect(), L"Connected to MQTT 7");
            Assert::AreEqual(ConnectionState::MqttConnected, connection.connect(), L"Connected to MQTT 8");
            Assert::AreEqual(ConnectionState::MqttConnected, connection.connect(), L"Connected to MQTT 9");
            Assert::AreEqual(ConnectionState::MqttReady, connection.connect(), L"Final MQTT Ready");
        }
    };

    EventServer ConnectionTest::eventServer;
    WifiMock ConnectionTest::wifiMock(&eventServer);
    PubSubClient ConnectionTest::mqttClient;
    MqttGatewayMock ConnectionTest::mqttGatewayMock(&eventServer, &mqttClient);
    TimeServerMock ConnectionTest::timeServer(&eventServer);
    FirmwareManager ConnectionTest::firmwareManager(&eventServer, "http://localhost", "0.99.3");
    PayloadBuilder ConnectionTest::payloadBuilder;
    Serializer ConnectionTest::serializer(&payloadBuilder);
    DataQueue ConnectionTest::dataQueue(&eventServer, &serializer);
    QueueHandle_t ConnectionTest::queueHandle = nullptr;
    QueueClient ConnectionTest::queueClient(&eventServer);
    Log ConnectionTest::logger(&eventServer);
    LedDriver ConnectionTest::ledDriver(&eventServer);
    Connector ConnectionTest::connection(&eventServer, &logger, &ledDriver, &wifiMock, &mqttGatewayMock, &timeServer, &firmwareManager, &dataQueue, &queueClient);
}
