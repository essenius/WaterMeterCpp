// Copyright 2021-2024 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

// ReSharper disable CyclomaticComplexity -- caused by EXPECT macros

#include "gtest/gtest.h"

#include <ESP.h>
#include "MqttGatewayMock.h"
#include "WiFiMock.h"
#include "Connector.h"
// ReSharper disable once CppUnusedIncludeDirective -- false positive
#include "TestEventClient.h"
#include "TimeServerMock.h"
#include "DataQueue.h"

namespace WaterMeterCppTest {
    using WaterMeter::Configuration;
    using WaterMeter::ConnectionState;
    using WaterMeter::Connector;
    using WaterMeter::DataQueue;
    using WaterMeter::DataQueuePayload;
    using WaterMeter::FirmwareConfig;
    using WaterMeter::FirmwareManager;
    using WaterMeter::Log;
    using WaterMeter::PayloadBuilder;
    using WaterMeter::QueueClient;
    using WaterMeter::Serializer;
    using WaterMeter::WiFiClientFactory;
    using WaterMeter::WifiInitialWaitDuration;
    using WaterMeter::WifiReconnectWaitDuration;
    using WaterMeter::MaxReconnectFailures;



    class ConnectorTest : public testing::Test {
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
        static QueueClient communicatorQueueClient;
        static DataQueue dataQueue;
        static DataQueue commsDataQueue;

        // ReSharper disable once CppInconsistentNaming
        static void SetUpTestCase() {
            disableDelay(false);
            setRealTime(false);
            queueClient1.begin(communicatorQueueClient.getQueueHandle());
            communicatorQueueClient.begin();
        }

        // Predicate function within the test file
        static void expectConnectWithState(const ConnectionState state, const char* message) {
            const auto result = connector.connect();
            EXPECT_TRUE(result == state) << message;
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
    FirmwareConfig firmwareConfig{"http://localhost/"};
    FirmwareManager ConnectorTest::firmwareManager(&eventServer, &wifiClientFactory, &firmwareConfig, "0.99.3");
    PayloadBuilder ConnectorTest::payloadBuilder;
    Serializer ConnectorTest::serializer(&eventServer, &payloadBuilder);
    DataQueuePayload payload;
    DataQueue ConnectorTest::dataQueue(&eventServer, &payload);
    DataQueue ConnectorTest::commsDataQueue(&eventServer, &payload, 1, 2, 1, 2);

    QueueHandle_t ConnectorTest::queueHandle = nullptr;
    QueueClient ConnectorTest::queueClient1(&eventServer, &logger, 10, 1);
    QueueClient ConnectorTest::communicatorQueueClient(&eventServer, &logger, 10, 2);
    Connector ConnectorTest::connector(&eventServer, &wifiMock, &mqttGatewayMock, &timeServer, &firmwareManager, &dataQueue,
                                       &commsDataQueue, &serializer, &queueClient1, &communicatorQueueClient);
    
    TEST_F(ConnectorTest, maxWifiFailuresTest) {
        EXPECT_STREQ("", getPrintOutput()) << "Print buffer empty start";
        connector.begin(&configuration);
        wifiMock.setIsConnected(false);

        EXPECT_EQ(ConnectionState::WifiConnecting, connector.loop()) << "Connecting";
        delay(WifiInitialWaitDuration / 1000);

        for (unsigned int i = 0; i < MaxReconnectFailures; i++) {
            EXPECT_EQ(ConnectionState::Disconnected, connector.loop()) << "Disconnected";
            EXPECT_EQ(ConnectionState::WifiConnecting, connector.loop()) << "Connecting 2";
            delay(WifiReconnectWaitDuration / 1000);
        }
        EXPECT_EQ(ConnectionState::Init, connector.loop()) << "Failed too many times) << re-init";
        EXPECT_STREQ("", getPrintOutput()) << "Print buffer empty end";

        EXPECT_FALSE(uxQueueMessagesWaiting(queueClient1.getQueueHandle())) << "queueClient1 ok";
        EXPECT_FALSE(uxQueueMessagesWaiting(communicatorQueueClient.getQueueHandle())) << "communicatorQueueClient ok";
    }

    TEST_F(ConnectorTest, reInitTest) {
        EXPECT_STREQ("", getPrintOutput()) << "Print buffer empty start";
        connector.begin(&configuration);
        wifiMock.setIsConnected(false);
        wifiMock.setNeedsReconnect(true);
        expectConnectWithState(ConnectionState::WifiConnecting, "Connecting");

        wifiMock.setIsConnected(true);
        expectConnectWithState(ConnectionState::WifiConnected, "Connected");
        expectConnectWithState(ConnectionState::Init, "Back to init");

        wifiMock.setNeedsReconnect(false);
        expectConnectWithState(ConnectionState::WifiConnecting,"Connecting");
        expectConnectWithState(ConnectionState::WifiConnected, "Connected");

        wifiMock.setIsConnected(false);
        expectConnectWithState(ConnectionState::Disconnected, "Disconnected");
        EXPECT_STREQ("", getPrintOutput()) << "Print buffer empty end";
        EXPECT_FALSE(uxQueueMessagesWaiting(queueClient1.getQueueHandle())) << "queueClient1 ok";
        EXPECT_FALSE(uxQueueMessagesWaiting(communicatorQueueClient.getQueueHandle())) << "communicatorQueueClient ok";
    }

    TEST_F(ConnectorTest, scriptTest) {
        EXPECT_STREQ("", getPrintOutput()) << "Print buffer empty start";
        timeServer.reset();
        // connect before timeout
        connector.begin(&configuration);
        wifiMock.setNeedsReconnect(false);
        wifiMock.setIsConnected(false);
        expectConnectWithState(ConnectionState::WifiConnecting, "Wifi connecting");
        expectConnectWithState(ConnectionState::WifiConnecting, "Wifi connecting 2");

        delay(3000);
        expectConnectWithState(ConnectionState::WifiConnecting, "Wifi connecting 3");

        wifiMock.setIsConnected(true);
        expectConnectWithState(ConnectionState::WifiConnected, "Wifi connected");
        expectConnectWithState(ConnectionState::RequestTime, "Set time");
        expectConnectWithState(ConnectionState::SettingTime, "Setting time");
        expectConnectWithState(ConnectionState::CheckFirmware, "Checking firmware");
        expectConnectWithState(ConnectionState::WifiReady, "Wifi ready");

        // happy path for mqtt
        mqttGatewayMock.setIsConnected(true);
        expectConnectWithState(ConnectionState::MqttConnecting, "Connecting to MQTT 1");
        expectConnectWithState(ConnectionState::MqttConnected, "Connected to MQTT 1");

        const auto timestampReady = micros();
        EXPECT_EQ(ConnectionState::MqttReady, connector.loop()) << "MQTT ready";

        // we're doing the division to filter out the few micros that the statement costs
        EXPECT_EQ(50UL, (micros() - timestampReady) / 1000UL) << "50 ms wait time when ready";
        expectConnectWithState(ConnectionState::MqttReady, "MQTT stays ready if nothing changes");

        // disconnecting Wi-Fi should change state to Disconnected
        wifiMock.setIsConnected(false);
        EXPECT_EQ(ConnectionState::Disconnected, connector.loop()) << "Disconnects if wifi is down";

        expectConnectWithState(ConnectionState::WifiConnecting, "Reconnecting Wifi");
        delay(20000);
        expectConnectWithState(ConnectionState::Disconnected, "Still down, disconnected");

        wifiMock.setIsConnected(true);
        expectConnectWithState(ConnectionState::WifiConnecting, "Connecting after Wifi comes up");
        expectConnectWithState(ConnectionState::WifiConnected, "Wifi reconnected");
        expectConnectWithState(ConnectionState::RequestTime, "SetTime 2 (does nothing)");
        expectConnectWithState(ConnectionState::CheckFirmware, "Checking firmware 2 (does nothing)");

        expectConnectWithState(ConnectionState::WifiReady, "Wifi Ready 2");
        expectConnectWithState(ConnectionState::MqttConnecting, "Connecting to MQTT 2");
        expectConnectWithState(ConnectionState::MqttConnected, "Connected to MQTT 2");
        expectConnectWithState(ConnectionState::MqttReady, "Mqtt Ready 2");

        // disconnecting MQTT should put the state to Wi-Fi connected
        mqttGatewayMock.setIsConnected(false);
        expectConnectWithState(ConnectionState::WifiReady, "back to Wifi Ready");
        expectConnectWithState(ConnectionState::MqttConnecting, "Connecting to MQTT 3");
        expectConnectWithState(ConnectionState::WaitingForMqttReconnect, "awaiting MQTT timeout");
        delay(2000);
        expectConnectWithState(ConnectionState::WifiReady, "done waiting, back to Wifi Ready");

        wifiMock.setIsConnected(false);
        expectConnectWithState(ConnectionState::Disconnected, "Wifi disconnected too");

        wifiMock.setIsConnected(true);
        mqttGatewayMock.setIsConnected(true);
        expectConnectWithState(ConnectionState::WifiConnecting, "Connecting Wifi 4");
        expectConnectWithState(ConnectionState::WifiConnected, "Wifi Connected 4");
        expectConnectWithState(ConnectionState::RequestTime, "SetTime 3(does nothing)");
        expectConnectWithState(ConnectionState::CheckFirmware, "Checking firmware 3 (does nothing)");

        expectConnectWithState(ConnectionState::WifiReady, "Wifi Ready 4");
        expectConnectWithState(ConnectionState::MqttConnecting, "Connecting to MQTT 4");
        expectConnectWithState(ConnectionState::MqttConnected, "Connected to MQTT 4");

        mqttGatewayMock.setIsConnected(false);
        expectConnectWithState(ConnectionState::WifiReady, "announce failed, to Wifi ready");

        mqttGatewayMock.setIsConnected(true);
        expectConnectWithState(ConnectionState::MqttConnecting, "Connecting to MQTT 5");
        expectConnectWithState(ConnectionState::MqttConnected, "Connected to MQTT 5");
        expectConnectWithState(ConnectionState::MqttReady, "Final MQTT Ready");

        EXPECT_STREQ("", getPrintOutput()) << "Print buffer empty end";
    }

    TEST_F(ConnectorTest, timeFailTest) {
        EXPECT_STREQ("", getPrintOutput()) << "Print buffer empty start";

        connector.begin(&configuration);
        wifiMock.setIsConnected(true);
        wifiMock.setNeedsReconnect(false);

        expectConnectWithState(ConnectionState::WifiConnecting, "Connecting");
        expectConnectWithState(ConnectionState::WifiConnected, "Connected");
        expectConnectWithState(ConnectionState::RequestTime, "Request time 1");

        // make sure time was not set
        timeServer.reset();
        expectConnectWithState(ConnectionState::SettingTime, "Waiting for time to be set 1");

        // and again, since the standard mock mechanism switches it on
        timeServer.reset();
        expectConnectWithState(ConnectionState::SettingTime, "Waiting for time to be set 2");
        delayMicroseconds(11000000);
        expectConnectWithState(ConnectionState::WifiConnected, "Back to Connected");
        expectConnectWithState(ConnectionState::RequestTime, "Request time 2");

        // now let timeServer succeed
        timeServer.setTime();
        expectConnectWithState(ConnectionState::CheckFirmware, "Time was set");
        EXPECT_STREQ("", getPrintOutput()) << "Print buffer empty end";
    }

    TEST_F(ConnectorTest, queueStringPayloadTest) {
        EXPECT_STREQ("", getPrintOutput()) << "Print buffer empty start";
        connector.begin(&configuration);
        constexpr char Buffer[] = "12345.6789012";
        EventServer receivingEventServer;
        QueueClient receivingQueueClient(&receivingEventServer, &logger, 20);
        communicatorQueueClient.begin(receivingQueueClient.getQueueHandle());
        receivingQueueClient.begin();
        eventServer.publish(Topic::SetVolume, Buffer);
        // now the entry is in the queue. Pick it up at the other end.
        TestEventClient client(&receivingEventServer);
        receivingEventServer.subscribe(&client, Topic::SetVolume);
        receivingQueueClient.receive();

        EXPECT_EQ(1, client.getCallCount()) << "Test client called";
        EXPECT_STREQ("12345.6789012", client.getPayload()) << "Payload ok";
        EXPECT_STREQ("", getPrintOutput()) << "Print buffer empty end";

        EXPECT_FALSE(uxQueueMessagesWaiting(queueClient1.getQueueHandle())) << "queueClient1 ok";
        EXPECT_FALSE(uxQueueMessagesWaiting(communicatorQueueClient.getQueueHandle())) << "communicatorQueueClient ok";
        // make sure the client has no send queue for the other tests
        communicatorQueueClient.begin();
        EXPECT_FALSE(uxQueueMessagesWaiting(queueClient1.getQueueHandle())) << "queueClient1 ok";
        EXPECT_FALSE(uxQueueMessagesWaiting(communicatorQueueClient.getQueueHandle())) << "communicatorQueueClient ok";

        EXPECT_STREQ("", getPrintOutput()) << "Print buffer empty end";
    }

    TEST_F(ConnectorTest, wifiInitTest) {
        EXPECT_STREQ("", getPrintOutput()) << "Print buffer empty start";
        connector.begin(&configuration);

        wifiMock.setNeedsReconnect(true);
        wifiMock.setIsConnected(true);
        timeServer.reset();
        expectConnectWithState(ConnectionState::WifiConnecting, "Connecting");
        expectConnectWithState(ConnectionState::WifiConnected, "Connected");
        expectConnectWithState(ConnectionState::Init, "Disconnected (reconnect)");

        wifiMock.setNeedsReconnect(false);
        expectConnectWithState(ConnectionState::WifiConnecting, "Reconnecting");
        expectConnectWithState(ConnectionState::WifiConnected, "Reconnected");
        expectConnectWithState(ConnectionState::RequestTime, "Requesting time");

        // disconnected just after time was requested. Time still comes in
        wifiMock.setIsConnected(false);
        expectConnectWithState(ConnectionState::SettingTime, "Setting time");
        expectConnectWithState(ConnectionState::Disconnected, "Disconnected");
        expectConnectWithState(ConnectionState::WifiConnecting, "Connecting");
        expectConnectWithState(ConnectionState::WifiConnecting, "Still Connecting");

        wifiMock.setIsConnected(true);
        expectConnectWithState(ConnectionState::WifiConnected, "Connected 2");
        expectConnectWithState(ConnectionState::RequestTime, "Requesting time 2");
        expectConnectWithState(ConnectionState::CheckFirmware, "Checking firmware");
        EXPECT_STREQ("", getPrintOutput()) << "Print buffer empty end";
    }
}