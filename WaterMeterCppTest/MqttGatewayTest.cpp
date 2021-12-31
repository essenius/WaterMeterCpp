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
#include "CppUnitTest.h"
#include "TestEventClient.h"
#include "../WaterMeterCpp/MqttGateway.h"
#include "../WaterMeterCpp/EventServer.h"
//#include "TopicHelper.h"
#include "../WaterMeterCpp/secrets.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

extern PubSubClient mqttClient;


namespace WaterMeterCppTest {


    TEST_CLASS(MqttGatewayTest) {
    public:
        static Client Client;

        static EventServer EventServer;
        static TestEventClient DisconnectedListener;
        static TestEventClient ConnectedListener;
        static TestEventClient ErrorListener;
        static TestEventClient InfoListener;

        TEST_CLASS_INITIALIZE(mqttGatewayClassInitialize) {
            EventServer.subscribe(&ConnectedListener, Topic::Connected);
            EventServer.subscribe(&DisconnectedListener, Topic::Disconnected);
            EventServer.subscribe(&ErrorListener, Topic::Error);
            EventServer.subscribe(&InfoListener, Topic::Info);
            mqttClient.reset();
        }

        TEST_METHOD_INITIALIZE(mqttGatewayMethodInitialize) {
            mqttClient.reset();
            DisconnectedListener.reset();
            ConnectedListener.reset();
            ErrorListener.reset();
            InfoListener.reset();
        }

        TEST_CLASS_CLEANUP(wifiTestClassCleanup) {
            EventServer.unsubscribe(&ConnectedListener);
            EventServer.unsubscribe(&DisconnectedListener);
            EventServer.unsubscribe(&InfoListener);
            EventServer.unsubscribe(&ErrorListener);
        }

        TEST_METHOD(mqttGatewayScriptTest) {
            // We need to make this a longer test since the init needs to be done for the rest to work

            // Init part
            MqttGateway gateway(&EventServer, CONFIG_MQTT_BROKER, CONFIG_MQTT_PORT, CONFIG_MQTT_USER,
                                CONFIG_MQTT_PASSWORD);

            gateway.begin(&Client, "client1");
            // first check if the connection event was sent (no disconnects, one connect - no more)
            Assert::AreEqual(0, DisconnectedListener.getCallCount(), L"no disconnected event");
            Assert::AreEqual(1, ConnectedListener.getCallCount(), L"connected event");
            Assert::AreEqual(1, ErrorListener.getCallCount(), L"Error called once");
            Assert::AreEqual("", ErrorListener.getPayload(), L"Error empty");
            Assert::AreEqual(3, InfoListener.getCallCount(), L"Info called three times");
            Assert::AreEqual("MQTT: Announcement complete", InfoListener.getPayload(), L"Info last message correct");

            Assert::AreEqual(CONFIG_MQTT_USER, mqttClient.user());
            Assert::AreEqual("client1", mqttClient.id());
            // check if the homie init events were sent 1586 1613
            Assert::AreEqual(static_cast<size_t>(1656), strlen(mqttClient.getTopics()), L"Topic lenght OK");
            Assert::AreEqual(static_cast<size_t>(473), strlen(mqttClient.getPayloads()), L"Payload lenght OK");
            Assert::AreEqual(46, mqttClient.getCallCount());
            mqttClient.reset();

            // Valid callback test
            TestEventClient callBackListener("disconnectedListener", &EventServer);
            EventServer.subscribe(&callBackListener, Topic::BatchSizeDesired);
            char topic[100];
            constexpr int PAYLOAD_SIZE = 2;
            uint8_t payload[PAYLOAD_SIZE] = {'2', '0'};
            strcpy(topic, "homie/device_id/measurement/batch-size-desired/set");
            mqttClient.callBack(topic, payload, PAYLOAD_SIZE);
            Assert::AreEqual(1, callBackListener.getCallCount(), L"callBackListener called");
            Assert::AreEqual("20", callBackListener.getPayload(), L"callBackListener got right payload");

            callBackListener.reset();
            strcpy(topic, "homie/device_id/measurement/batch-size-desired/get");
            mqttClient.callBack(topic, payload, PAYLOAD_SIZE);
            Assert::AreEqual(0, callBackListener.getCallCount(), L"callBackListener not called");
            callBackListener.reset();

            // empty topic callback test. Should not do anything, just checking nothing breaks
            topic[0] = 0;
            mqttClient.callBack(topic, payload, PAYLOAD_SIZE);

            // same with a payload not having a device id
            strcpy(topic, "bogus");
            mqttClient.callBack(topic, payload, PAYLOAD_SIZE);

            // a topic we don't know should be ignored
            strcpy(topic, "homie/device_id/bogus/batch-size-desired/set");
            mqttClient.callBack(topic, payload, PAYLOAD_SIZE);
            Assert::AreEqual(0, callBackListener.getCallCount(), L"callBackListener not called");
        }

        TEST_METHOD(mqttGatewayNoUserTest) {
            MqttGateway gateway(&EventServer, CONFIG_MQTT_BROKER, CONFIG_MQTT_PORT, nullptr, "");
            gateway.begin(&Client, "client1", false);
            gateway.connect();
            Assert::AreEqual("", mqttClient.user(), "User not set");
        }

        TEST_METHOD(mqttGatewayCannotConnectTest) {
            mqttClient.setCanConnect(false);
            MqttGateway gateway(&EventServer, CONFIG_MQTT_BROKER, CONFIG_MQTT_PORT, CONFIG_MQTT_USER,
                                CONFIG_MQTT_PASSWORD);
            gateway.begin(&Client, "client1");
            Assert::AreEqual(0, DisconnectedListener.getCallCount(), L"Disconnected published");
            Assert::AreEqual(0, ConnectedListener.getCallCount(), L"Connected not published");
            Assert::AreEqual("MQTT: Connecting", InfoListener.getPayload(), "tried to connect");
            Assert::AreEqual("MQTT: Could not connect to broker [state = 3]", ErrorListener.getPayload(),
                             "Error happened");
        }

        TEST_METHOD(mqttGatewayCannotSubscribeTest) {
            mqttClient.setCanSubscribe(false);
            MqttGateway gateway(&EventServer, CONFIG_MQTT_BROKER, CONFIG_MQTT_PORT, CONFIG_MQTT_USER,
                                CONFIG_MQTT_PASSWORD);
            gateway.begin(&Client, "client1");
            Assert::AreEqual(1, DisconnectedListener.getCallCount(), L"Disconnected published");
            Assert::AreEqual(1, ConnectedListener.getCallCount(), L"Connected published");
            Assert::AreEqual("MQTT: Connecting", InfoListener.getPayload(), "tried to connect");
            Assert::AreEqual("MQTT: Could not subscribe to setters [state = 3]", ErrorListener.getPayload(),
                             "Error happened");
        }

        TEST_METHOD(mqttGatewayCannotAnnounceTest) {
            mqttClient.setCanPublish(false);
            MqttGateway gateway(&EventServer, CONFIG_MQTT_BROKER, CONFIG_MQTT_PORT, CONFIG_MQTT_USER,
                                CONFIG_MQTT_PASSWORD);
            gateway.begin(&Client, "client1");
            Assert::AreEqual(1, DisconnectedListener.getCallCount(), L"Disconnected published");
            Assert::AreEqual(1, ConnectedListener.getCallCount(), L"Connected published");
            Assert::AreEqual("MQTT: Connected and subscribed to setters", InfoListener.getPayload(),
                             "tried to connect");
            Assert::AreEqual("MQTT: Could not announce device [state = 3]", ErrorListener.getPayload(),
                             "Error happened");
        }

        TEST_METHOD(mqttGatewayConnectionLossTest) {
            MqttGateway gateway(&EventServer, CONFIG_MQTT_BROKER, CONFIG_MQTT_PORT, CONFIG_MQTT_USER,
                                CONFIG_MQTT_PASSWORD);
            gateway.begin(&Client, "client1");
            Assert::AreEqual(0, DisconnectedListener.getCallCount(), L"No disconnect published");
            Assert::AreEqual(1, ConnectedListener.getCallCount(), L"Connected published");

            mqttClient.setCanConnect(false);
            DisconnectedListener.reset();
            ConnectedListener.reset();
            ErrorListener.reset();
            // force an evaluation of the connection state
            EventServer.publish(Topic::FreeHeap, 1000);
            Assert::AreEqual("MQTT: Could not connect to broker [state = 3]", ErrorListener.getPayload(),
                             "Error happened");
            Assert::AreEqual(1, DisconnectedListener.getCallCount(), L"Disconnected published");

            InfoListener.reset();
            ErrorListener.reset();
            // try connecting again
            gateway.handleQueue();
            Assert::AreEqual(0, ErrorListener.getCallCount(), L"Error not called again");
            Assert::AreEqual(0, InfoListener.getCallCount(), L"Info not called again (i.e. no re-init");

            delay(1000);
            gateway.handleQueue();
            Assert::AreEqual("MQTT: Connecting", InfoListener.getPayload(), L"Re-init after one second");
            Assert::AreEqual("MQTT: Could not connect to broker [state = 3]", ErrorListener.getPayload(),
                             L"Reconnect failed");
            mqttClient.setCanConnect(true);
            ConnectedListener.reset();
            InfoListener.reset();
            ErrorListener.reset();
            delay(1000);
            gateway.handleQueue();
            Assert::AreEqual(1, ConnectedListener.getCallCount(), L"reconnected");
            Assert::AreEqual("MQTT: Announcement complete", InfoListener.getPayload(), L"Re-initialized");
            Assert::AreEqual("", ErrorListener.getPayload(), L"Error reset after reconnect");


        }


    };

    Client MqttGatewayTest::Client;
    EventServer MqttGatewayTest::EventServer(LogLevel::Off);
    TestEventClient MqttGatewayTest::DisconnectedListener("disconnectedListener", &EventServer);
    TestEventClient MqttGatewayTest::ConnectedListener("connectedListener", &EventServer);
    TestEventClient MqttGatewayTest::ErrorListener("errorListener", &EventServer);
    TestEventClient MqttGatewayTest::InfoListener("infoListener", &EventServer);
}
