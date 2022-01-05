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
#include <iostream>

#include "../WaterMeterCpp/secrets.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

extern PubSubClient mqttClient;


namespace WaterMeterCppTest {


    constexpr const char* const BROKER = "broker";
    constexpr const char* const PASSWORD = "password";
    constexpr int PORT = 1883;
    constexpr const char* const USER = "user";
    constexpr const char* const BUILD = "1";

    TEST_CLASS(MqttGatewayTest) {
    public:
        static Client Client;

        static EventServer EventServer;
        static TestEventClient ErrorListener;
        static TestEventClient InfoListener;

        TEST_CLASS_INITIALIZE(mqttGatewayClassInitialize) {
            EventServer.subscribe(&ErrorListener, Topic::Error);
            EventServer.subscribe(&InfoListener, Topic::Info);
            mqttClient.reset();
        }

        TEST_METHOD_INITIALIZE(mqttGatewayMethodInitialize) {
            mqttClient.reset();
            ErrorListener.reset();
            InfoListener.reset();
        }

        TEST_CLASS_CLEANUP(wifiTestClassCleanup) {
            EventServer.unsubscribe(&InfoListener);
            EventServer.unsubscribe(&ErrorListener);
        }

        /*TEST_METHOD(mqttGatewayCannotAnnounceTest) {
            mqttClient.setCanPublish(false);
            MqttGateway gateway(&EventServer, BROKER, PORT, USER, PASSWORD, BUILD);
            gateway.begin(&Client, "client1");
            Assert::AreEqual("MQTT: Could not announce device [state = 3]", ErrorListener.getPayload(),
                "Error happened");
        }*/

        /*TEST_METHOD(mqttGatewayCannotConnectTest) {
            mqttClient.setCanConnect(false);
            MqttGateway gateway(&EventServer, BROKER, PORT, USER, PASSWORD, BUILD);
            gateway.begin(&Client, "client1");
            Assert::AreEqual("MQTT: Could not connect to broker [state = 3]", ErrorListener.getPayload(),
                "Error happened");
        }*/

        TEST_METHOD(mqttGatewayCannotSubscribeTest) {
            mqttClient.setCanSubscribe(false);
            MqttGateway gateway(&EventServer, BROKER, PORT, USER, PASSWORD, BUILD);
            gateway.begin(&Client, "client1");
            Assert::AreEqual("MQTT: Could not subscribe to setters [state = 3]", ErrorListener.getPayload(),
                "Error happened");
        }

        /*TEST_METHOD(mqttGatewayConnectionLossTest) {
            MqttGateway gateway(&EventServer, BROKER, PORT, USER, PASSWORD, BUILD);
            gateway.begin(&Client, "client1");

            mqttClient.setCanConnect(false);
            ErrorListener.reset();
            // force an evaluation of the connection state
            EventServer.publish(Topic::FreeHeap, 1000);

            ErrorListener.reset();
            // try connecting again
            gateway.handleQueue();
            Assert::AreEqual(0, ErrorListener.getCallCount(), L"Error not called again");

            delay(1000);
            gateway.handleQueue();
            Assert::AreEqual("MQTT: Could not connect to broker [state = 3]", ErrorListener.getPayload(),
                L"Reconnect failed");
            mqttClient.setCanConnect(true);
            InfoListener.reset();
            ErrorListener.reset();
            delay(1000);
            gateway.handleQueue();
            Assert::AreEqual("", ErrorListener.getPayload(), L"Error reset after reconnect");
        }*/

        TEST_METHOD(mqttGatewayNoUserTest) {
            MqttGateway gateway(&EventServer, BROKER, PORT, nullptr, "", BUILD);
            gateway.begin(&Client, "client1");
            Assert::AreEqual("", mqttClient.user(), "User not set");
        }


        TEST_METHOD(mqttGatewayScriptTest) {
            // We need to make this a longer test since the init needs to be done for the rest to work

            // Init part
            MqttGateway gateway(&EventServer, BROKER, PORT, USER, PASSWORD, BUILD);

            gateway.begin(&Client, "client1");
            // first check if the connection event was sent (no disconnects, one connect - no more)
            int count = 0;
            while (gateway.hasAnnouncement()) {
                Assert::IsTrue(gateway.publishNextAnnouncement(), (wchar_t *)(mqttClient.getPayloads()));
                count++;
            }
            Assert::AreEqual(54, count, L"announcement count");
            gateway.publishNextAnnouncement();
            Assert::AreEqual(0, ErrorListener.getCallCount(), L"Error not called");
            Assert::AreEqual(0, InfoListener.getCallCount(), L"Info not called");

            Assert::AreEqual(USER, mqttClient.user());
            Assert::AreEqual("client1", mqttClient.id());
            // check if the homie init events were sent 
            Assert::AreEqual(static_cast<size_t>(2005), strlen(mqttClient.getTopics()), L"Topic lenght OK");
            Assert::AreEqual(static_cast<size_t>(564), strlen(mqttClient.getPayloads()), L"Payload lenght OK");
            Assert::AreEqual(54, mqttClient.getCallCount(), L"Call count");

            gateway.announceReady();

            mqttClient.reset();

            // Incoming event from event server hould get published

            EventServer.publish(Topic::Rate, 7);
            Assert::AreEqual("homie/client1/result/rate\n", mqttClient.getTopics(), L"Payload OK");
            Assert::AreEqual("7\n", mqttClient.getPayloads(), L"Payload OK");

            // Incoming valid callback from MQTT should get passed on to the event server

            TestEventClient callBackListener("disconnectedListener", &EventServer);
            EventServer.subscribe(&callBackListener, Topic::BatchSizeDesired);
            char topic[100];
            constexpr int PAYLOAD_SIZE = 2;
            uint8_t payload[PAYLOAD_SIZE] = {'2', '0'};
            strcpy(topic, "homie/device_id/measurement/batch-size-desired/set");
            mqttClient.callBack(topic, payload, PAYLOAD_SIZE);
            Assert::AreEqual(1, callBackListener.getCallCount(), L"callBackListener called");
            Assert::AreEqual("20", callBackListener.getPayload(), L"callBackListener got right payload");

            // Invalid callback should get ignored

            callBackListener.reset();
            strcpy(topic, "homie/device_id/measurement/batch-size-desired/get");
            mqttClient.callBack(topic, payload, PAYLOAD_SIZE);
            Assert::AreEqual(0, callBackListener.getCallCount(), L"callBackListener not called");
            callBackListener.reset();

            // Empty topic should get ignored, just checking nothing breaks

            topic[0] = 0;
            mqttClient.callBack(topic, payload, PAYLOAD_SIZE);

            // same with a payload not having a device id

            strcpy(topic, "bogus");
            mqttClient.callBack(topic, payload, PAYLOAD_SIZE);

            // a topic we don't know should be ignored

            strcpy(topic, "homie/device_id/bogus/batch-size-desired/set");
            mqttClient.callBack(topic, payload, PAYLOAD_SIZE);
            Assert::AreEqual(0, callBackListener.getCallCount(), L"callBackListener not called");

            gateway.handleQueue();
            Assert::AreEqual(1, mqttClient.getLoopCount(), L"Loop count 1");
            mqttClient.setCanConnect(false);
            gateway.handleQueue();
            Assert::AreEqual(1, mqttClient.getLoopCount(), L"Loop count still 1 after disconnect");

        }

    };

    Client MqttGatewayTest::Client;
    EventServer MqttGatewayTest::EventServer(LogLevel::Off);
    TestEventClient MqttGatewayTest::ErrorListener("errorListener", &EventServer);
    TestEventClient MqttGatewayTest::InfoListener("infoListener", &EventServer);
}
