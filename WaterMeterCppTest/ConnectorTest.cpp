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

// ReSharper disable CyclomaticComplexity -- caused by EXPECT macros

#include "gtest/gtest.h"

#include <ESP.h>
#include "MqttGatewayMock.h"
#include "WiFiMock.h"
#include "../WaterMeterCpp/Connector.h"
// ReSharper disable once CppUnusedIncludeDirective -- false positive
#include "TestEventClient.h"
#include "TimeServerMock.h"
#include "../WaterMeterCpp/DataQueue.h"

namespace WaterMeterCppTest {

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
    
    TEST_F(ConnectorTest, connectorMaxWifiFailuresTest) {
        EXPECT_STREQ("", getPrintOutput()) << "Print buffer empty start";
        connector.begin(&configuration);
        wifiMock.setIsConnected(false);

        EXPECT_EQ(ConnectionState::WifiConnecting, connector.loop()) << "Connecting";
        delay(WIFI_INITIAL_WAIT_DURATION / 1000);

        for (unsigned int i = 0; i < MAX_RECONNECT_FAILURES; i++) {
            EXPECT_EQ(ConnectionState::Disconnected, connector.loop()) << "Disconnected";
            EXPECT_EQ(ConnectionState::WifiConnecting, connector.loop()) << "Connecting 2";
            delay(WIFI_RECONNECT_WAIT_DURATION / 1000);
        }
        EXPECT_EQ(ConnectionState::Init, connector.loop()) << "Failed too many times) << re-init";
        EXPECT_STREQ("", getPrintOutput()) << "Print buffer empty end";

        EXPECT_FALSE(uxQueueMessagesWaiting(queueClient1.getQueueHandle())) << "queueClient1 ok";
        EXPECT_FALSE(uxQueueMessagesWaiting(communicatorQueueClient.getQueueHandle())) << "communicatorQueueClient ok";
    }

    TEST_F(ConnectorTest, connectorReInitTest) {
        EXPECT_STREQ("", getPrintOutput()) << "Print buffer empty start";
        connector.begin(&configuration);
        wifiMock.setIsConnected(false);
        wifiMock.setNeedsReconnect(true);
        EXPECT_EQ(ConnectionState::WifiConnecting, connector.connect()) << "Connecting";
        wifiMock.setIsConnected(true);

        EXPECT_EQ(ConnectionState::WifiConnected, connector.connect()) << "Connected";
        EXPECT_EQ(ConnectionState::Init, connector.connect()) << "Back to init";
        wifiMock.setNeedsReconnect(false);
        EXPECT_EQ(ConnectionState::WifiConnecting, connector.connect()) << "Connecting";
        EXPECT_EQ(ConnectionState::WifiConnected, connector.connect()) << "Connected";
        wifiMock.setIsConnected(false);
        EXPECT_EQ(ConnectionState::Disconnected, connector.connect()) << "Disconnected";
        EXPECT_STREQ("", getPrintOutput()) << "Print buffer empty end";

        EXPECT_FALSE(uxQueueMessagesWaiting(queueClient1.getQueueHandle())) << "queueClient1 ok";
        EXPECT_FALSE(uxQueueMessagesWaiting(communicatorQueueClient.getQueueHandle())) << "communicatorQueueClient ok";
    }

    TEST_F(ConnectorTest, connectorScriptTest) {
        EXPECT_STREQ("", getPrintOutput()) << "Print buffer empty start";
        timeServer.reset();
        // connect before timeout
        connector.begin(&configuration);
        wifiMock.setNeedsReconnect(false);
        wifiMock.setIsConnected(false);
        EXPECT_EQ(ConnectionState::WifiConnecting, connector.connect()) << "Wifi connecting";

        EXPECT_EQ(ConnectionState::WifiConnecting, connector.connect()) << "Wifi connecting 2";
        delay(3000);
        EXPECT_EQ(ConnectionState::WifiConnecting, connector.connect()) << "Wifi connecting 3";

        wifiMock.setIsConnected(true);
        EXPECT_EQ(ConnectionState::WifiConnected, connector.connect()) << "Wifi connected";
        EXPECT_EQ(ConnectionState::RequestTime, connector.connect()) << "Set time";

        EXPECT_EQ(ConnectionState::SettingTime, connector.connect()) << "Setting time";
        EXPECT_EQ(ConnectionState::CheckFirmware, connector.connect()) << "Checking firmware";
        EXPECT_EQ(ConnectionState::WifiReady, connector.connect()) << "Wifi ready";

        // happy path for mqtt
        mqttGatewayMock.setIsConnected(true);
        EXPECT_EQ(ConnectionState::MqttConnecting, connector.connect()) << "Connecting to MQTT 1";
        EXPECT_EQ(ConnectionState::MqttConnected, connector.connect()) << "Connected to MQTT 1";
        const auto timestampReady = micros();
        EXPECT_EQ(ConnectionState::MqttReady, connector.loop()) << "MQTT ready";

        // we're doing the division to filter out the few micros that the statement costs
        EXPECT_EQ(50UL, (micros() - timestampReady) / 1000UL) << "50 ms wait time when ready";
        EXPECT_EQ(ConnectionState::MqttReady, connector.connect()) << "MQTT stays ready if nothing changes";

        // disconnecting Wifi should change state to Disconnected
        wifiMock.setIsConnected(false);
        EXPECT_EQ(ConnectionState::Disconnected, connector.loop()) << "Disconnects if wifi is down";

        EXPECT_EQ(ConnectionState::WifiConnecting, connector.connect()) << "Reconnecting Wifi";
        delay(20000);
        EXPECT_EQ(ConnectionState::Disconnected, connector.connect()) << "Still down, disconnected";
        wifiMock.setIsConnected(true);
        EXPECT_EQ(ConnectionState::WifiConnecting, connector.connect()) << "Connecting after Wifi comes up";
        EXPECT_EQ(ConnectionState::WifiConnected, connector.connect()) << "Wifi reconnected";
        EXPECT_EQ(ConnectionState::RequestTime, connector.connect()) << "SetTime 2 (does nothing)";
        EXPECT_EQ(ConnectionState::CheckFirmware, connector.connect()) << "Checking firmware 2 (does nothing)";

        EXPECT_EQ(ConnectionState::WifiReady, connector.connect()) << "Wifi Ready 2";
        EXPECT_EQ(ConnectionState::MqttConnecting, connector.connect()) << "Connecting to MQTT 2";
        EXPECT_EQ(ConnectionState::MqttConnected, connector.connect()) << "Connected to MQTT 2";
        EXPECT_EQ(ConnectionState::MqttReady, connector.connect()) << "Mqtt Ready 2";

        // disconnecting MQTT should put the state to Wifi connected
        mqttGatewayMock.setIsConnected(false);
        EXPECT_EQ(ConnectionState::WifiReady, connector.connect()) << "back to Wifi Ready";
        EXPECT_EQ(ConnectionState::MqttConnecting, connector.connect()) << "Connecting to MQTT 3";
        EXPECT_EQ(ConnectionState::WaitingForMqttReconnect, connector.connect()) << "awaiting MQTT timeout";
        delay(2000);
        EXPECT_EQ(ConnectionState::WifiReady, connector.connect()) << "done waiting, back to Wifi Ready";
        wifiMock.setIsConnected(false);
        EXPECT_EQ(ConnectionState::Disconnected, connector.connect()) << "Wifi disconnected too";
        wifiMock.setIsConnected(true);
        mqttGatewayMock.setIsConnected(true);
        EXPECT_EQ(ConnectionState::WifiConnecting, connector.connect()) << "Connecting Wifi 4";
        EXPECT_EQ(ConnectionState::WifiConnected, connector.connect()) << "Wifi Connected 4";
        EXPECT_EQ(ConnectionState::RequestTime, connector.connect()) << "SetTime 3(does nothing)";
        EXPECT_EQ(ConnectionState::CheckFirmware, connector.connect()) << "Checking firmware 3 (does nothing)";

        EXPECT_EQ(ConnectionState::WifiReady, connector.connect()) << "Wifi Ready 4";
        EXPECT_EQ(ConnectionState::MqttConnecting, connector.connect()) << "Connecting to MQTT 4";
        EXPECT_EQ(ConnectionState::MqttConnected, connector.connect()) << "Connected to MQTT 4";
        mqttGatewayMock.setIsConnected(false);
        EXPECT_EQ(ConnectionState::WifiReady, connector.connect()) << "announce failed, to Wifi ready";
        mqttGatewayMock.setIsConnected(true);
        EXPECT_EQ(ConnectionState::MqttConnecting, connector.connect()) << "Connecting to MQTT 5";
        EXPECT_EQ(ConnectionState::MqttConnected, connector.connect()) << "Connected to MQTT 5";
        EXPECT_EQ(ConnectionState::MqttReady, connector.connect()) << "Final MQTT Ready";

        EXPECT_STREQ("", getPrintOutput()) << "Print buffer empty end";
    }

    TEST_F(ConnectorTest, connectorTimeFailTest) {
        EXPECT_STREQ("", getPrintOutput()) << "Print buffer empty start";

        connector.begin(&configuration);
        wifiMock.setIsConnected(true);
        wifiMock.setNeedsReconnect(false);

        EXPECT_EQ(ConnectionState::WifiConnecting, connector.connect()) << "Connecting";
        EXPECT_EQ(ConnectionState::WifiConnected, connector.connect()) << "Connected";
        EXPECT_EQ(ConnectionState::RequestTime, connector.connect()) << "Request time 1";
        // make sure time was not set
        timeServer.reset();
        EXPECT_EQ(ConnectionState::SettingTime, connector.connect()) << "Waiting for time to be set 1";
        // and again, since the standard mock mechanism switches it on
        timeServer.reset();
        EXPECT_EQ(ConnectionState::SettingTime, connector.connect()) << "Waiting for time to be set 2";
        delayMicroseconds(11000000);
        EXPECT_EQ(ConnectionState::WifiConnected, connector.connect()) << "Back to Connected";
        EXPECT_EQ(ConnectionState::RequestTime, connector.connect()) << "Request time 2";
        // now let timeServer succeed
        timeServer.setTime();
        EXPECT_EQ(ConnectionState::CheckFirmware, connector.connect()) << "Time was set";
        EXPECT_STREQ("", getPrintOutput()) << "Print buffer empty end";
    }

    TEST_F(ConnectorTest, connectorQueueStringPayloadTest) {
        EXPECT_STREQ("", getPrintOutput()) << "Print buffer empty start";
        connector.begin(&configuration);
        constexpr char BUFFER[] = "12345.6789012";
        EventServer receivingEventServer;
        QueueClient receivingQueueClient(&receivingEventServer, &logger, 20);
        communicatorQueueClient.begin(receivingQueueClient.getQueueHandle());
        receivingQueueClient.begin();
        eventServer.publish(Topic::SetVolume, BUFFER);
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

    TEST_F(ConnectorTest, connectorWifiInitTest) {
        EXPECT_STREQ("", getPrintOutput()) << "Print buffer empty start";
        connector.begin(&configuration);

        wifiMock.setNeedsReconnect(true);
        wifiMock.setIsConnected(true);
        timeServer.reset();

        EXPECT_EQ(ConnectionState::WifiConnecting, connector.connect()) << "Connecting";
        EXPECT_EQ(ConnectionState::WifiConnected, connector.connect()) << "Connected";
        EXPECT_EQ(ConnectionState::Init, connector.connect()) << "Disconnected (reconnect)";
        wifiMock.setNeedsReconnect(false);
        EXPECT_EQ(ConnectionState::WifiConnecting, connector.connect()) << "Reconnecting";
        EXPECT_EQ(ConnectionState::WifiConnected, connector.connect()) << "Reconnected";
        EXPECT_EQ(ConnectionState::RequestTime, connector.connect()) << "Requesting time";
        // disconnected just after time was requested. Time still comes in
        wifiMock.setIsConnected(false);
        EXPECT_EQ(ConnectionState::SettingTime, connector.connect()) << "Setting time";

        EXPECT_EQ(ConnectionState::Disconnected, connector.connect()) << "Disconnected";
        EXPECT_EQ(ConnectionState::WifiConnecting, connector.connect()) << "Connecting";
        EXPECT_EQ(ConnectionState::WifiConnecting, connector.connect()) << "Still Connecting";
        wifiMock.setIsConnected(true);
        EXPECT_EQ(ConnectionState::WifiConnected, connector.connect()) << "Connected 2";
        EXPECT_EQ(ConnectionState::RequestTime, connector.connect()) << "Requesting time 2";
        EXPECT_EQ(ConnectionState::CheckFirmware, connector.connect()) << "Checking firmware";
        EXPECT_STREQ("", getPrintOutput()) << "Print buffer empty end";
    }
}