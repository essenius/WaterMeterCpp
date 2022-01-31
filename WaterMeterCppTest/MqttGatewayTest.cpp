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
#include "CppUnitTest.h"
#include "TestEventClient.h"
#include "../WaterMeterCpp/PubSubClientMock.h"
#include "../WaterMeterCpp/MqttGateway.h"
#include "../WaterMeterCpp/EventServer.h"
#include "../WaterMeterCpp/PayloadBuilder.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {

    constexpr MqttConfig MQTT_CONFIG_WITH_USER{ "broker", 1883, "user", "password" };
    constexpr MqttConfig MQTT_CONFIG_NO_USER{ "broker", 1883, nullptr, "" };

    constexpr const char* const BUILD = "1";

    TEST_CLASS(MqttGatewayTest) {
    public:
        static Client client;

        static EventServer eventServer;
        static Clock theClock;
        static TestEventClient errorListener;
        static TestEventClient infoListener;
        static PubSubClient mqttClient;
        static PayloadBuilder payloadBuilder;
        static Serializer serializer;
        static DataQueue dataQueue;

        TEST_CLASS_INITIALIZE(mqttGatewayClassInitialize) {
            eventServer.subscribe(&errorListener, Topic::CommunicationError);
            eventServer.subscribe(&infoListener, Topic::Info);
            mqttClient.reset();
        }

        TEST_METHOD_INITIALIZE(mqttGatewayMethodInitialize) {
            mqttClient.reset();
            errorListener.reset();
            infoListener.reset();
        }

        TEST_CLASS_CLEANUP(wifiTestClassCleanup) {
            eventServer.unsubscribe(&infoListener);
            eventServer.unsubscribe(&errorListener);
        }

        TEST_METHOD(mqttGatewayCannotSubscribeTest) {
            mqttClient.setCanSubscribe(false);
            MqttGateway gateway(&eventServer, &mqttClient, &MQTT_CONFIG_WITH_USER, &dataQueue, BUILD);
            gateway.begin(&client, "client1");
            Assert::AreEqual("MQTT: Could not subscribe to setters [state = 3]", errorListener.getPayload(),
                "Error happened");
        }

        TEST_METHOD(mqttGatewayNoUserTest) {
            
            MqttGateway gateway(&eventServer, &mqttClient, &MQTT_CONFIG_NO_USER, &dataQueue, BUILD);
            gateway.begin(&client, "client1");
            Assert::AreEqual("", mqttClient.user(), "User not set");
        }

        TEST_METHOD(mqttGatewayScriptTest) {
            // We need to make this a longer test since the init needs to be done for the rest to work

            // Init part
            MqttGateway gateway(&eventServer, &mqttClient, &MQTT_CONFIG_WITH_USER, &dataQueue, BUILD);

            gateway.begin(&client, "client1");

            int count = 0;
            while (gateway.hasAnnouncement()) {
                Assert::IsTrue(gateway.publishNextAnnouncement(), (L"Announcement #" + std::to_wstring(count)).c_str());
                count++;
            }
            Assert::AreEqual(60, count, L"announcement count");
            gateway.publishNextAnnouncement();
            Assert::AreEqual(0, errorListener.getCallCount(), L"Error not called");
            Assert::AreEqual(0, infoListener.getCallCount(), L"Info not called");

            Assert::AreEqual(MQTT_CONFIG_WITH_USER.user, mqttClient.user());
            Assert::AreEqual("client1", mqttClient.id());
            // check if the homie init events were sent 
            Assert::AreEqual(static_cast<size_t>(2307), strlen(mqttClient.getTopics()), L"Topic lenght OK");
            Assert::AreEqual(static_cast<size_t>(696), strlen(mqttClient.getPayloads()), L"Payload lenght OK");
            Assert::AreEqual(60, mqttClient.getCallCount(), L"Call count");

            gateway.announceReady();

            mqttClient.reset();

            // Incoming event from event server should get published

            eventServer.publish(Topic::Rate, 7);
            Assert::AreEqual("homie/client1/result/rate\n", mqttClient.getTopics(), L"Payload OK");
            Assert::AreEqual("7\n", mqttClient.getPayloads(), L"Payload OK");

            // Incoming valid callback from MQTT should get passed on to the event server

            TestEventClient callBackListener(&eventServer);
            eventServer.subscribe(&callBackListener, Topic::BatchSizeDesired);
            char topic[100];
            constexpr int PAYLOAD_SIZE = 2;
            uint8_t payload[PAYLOAD_SIZE] = {'2', '0'};
            safeStrcpy(topic, "homie/device_id/measurement/batch-size-desired/set");
            mqttClient.callBack(topic, payload, PAYLOAD_SIZE);
            Assert::AreEqual(1, callBackListener.getCallCount(), L"callBackListener called");
            Assert::AreEqual("20", callBackListener.getPayload(), L"callBackListener got right payload");

            // Invalid callback should get ignored

            callBackListener.reset();
            safeStrcpy(topic, "homie/device_id/measurement/batch-size-desired/get");
            mqttClient.callBack(topic, payload, PAYLOAD_SIZE);
            Assert::AreEqual(0, callBackListener.getCallCount(), L"callBackListener not called");
            callBackListener.reset();

            // Empty topic should get ignored, just checking nothing breaks

            topic[0] = 0;
            mqttClient.callBack(topic, payload, PAYLOAD_SIZE);

            // same with a payload not having a device id

            safeStrcpy(topic, "bogus");
            mqttClient.callBack(topic, payload, PAYLOAD_SIZE);

            // a topic we don't know should be ignored

            safeStrcpy(topic, "homie/device_id/bogus/batch-size-desired/set");
            mqttClient.callBack(topic, payload, PAYLOAD_SIZE);
            Assert::AreEqual(0, callBackListener.getCallCount(), L"callBackListener not called");

            gateway.handleQueue();
            Assert::AreEqual(1, mqttClient.getLoopCount(), L"Loop count 1");
            mqttClient.setCanConnect(false);
            gateway.handleQueue();
            Assert::AreEqual(1, mqttClient.getLoopCount(), L"Loop count still 1 after disconnect");

            /*mqttClient.reset();
            mqttClient.setCanConnect(false);

            gateway.update(Topic::BatchSize, 23);
            Assert::AreEqual("", mqttClient.getPayloads(), L"Nothing sent");
            errorListener.reset();
            gateway.update(Topic::Error, "Error message");
            Assert::AreEqual("", mqttClient.getPayloads(), L"Nothing sent");
            mqttClient.setCanConnect(true);
            Assert::AreEqual("23\n", mqttClient.getPayloads(), L"sent payload OK");
            Assert::AreEqual("homie/client1/measurement/batch-size\n", mqttClient.getTopics(), L"sent topic OK");
            mqttClient.reset();

            Assert::IsTrue(dataQueue.receive(), L"Received item from queue");
            Assert::AreEqual(" Error: Error message\n", mqttClient.getPayloads() + 26, L"sent payload OK");
            Assert::AreEqual("homie/client1/device/error\n", mqttClient.getTopics(), L"sent topic OK");
            Assert::IsFalse(dataQueue.receive(), L"No more items in the quue");

            mqttClient.reset();
            eventServer.publish(Topic::Alert, 1);
            Assert::AreEqual("homie/client1/$state\n", mqttClient.getTopics(), L"alert topic OK");
            Assert::AreEqual("alert\n", mqttClient.getPayloads(), L"alert payload OK"); */

        }
    };

    Client MqttGatewayTest::client;
    EventServer MqttGatewayTest::eventServer;
    Clock MqttGatewayTest::theClock(&eventServer);
    TestEventClient MqttGatewayTest::errorListener(&eventServer);
    TestEventClient MqttGatewayTest::infoListener(&eventServer);
    PubSubClient MqttGatewayTest::mqttClient;
    PayloadBuilder MqttGatewayTest::payloadBuilder;
    Serializer MqttGatewayTest::serializer(&payloadBuilder);
    DataQueue MqttGatewayTest::dataQueue(&eventServer, &theClock, &serializer);
}
