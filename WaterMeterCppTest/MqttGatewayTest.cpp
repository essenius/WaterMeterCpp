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
#include "TopicHelper.h"
#include "../WaterMeterCpp/secrets.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

extern PubSubClient mqttClient;


namespace WaterMeterCppTest {

	Client client;
	EventServer eventServer(LogLevel::Off);
	TestEventClient disconnectedListener("disconnectedListener", &eventServer);
	TestEventClient connectedListener("connectedListener", &eventServer);
	TestEventClient errorListener("errorListener", &eventServer);
	TestEventClient infoListener("infoListener", &eventServer);

	TEST_CLASS(MqttGatewayTest) {
public:

	TEST_CLASS_INITIALIZE(mqttGatewayClassInitialize) {
		eventServer.subscribe(&connectedListener, Topic::Connected);
		eventServer.subscribe(&disconnectedListener, Topic::Disconnected);
		eventServer.subscribe(&errorListener, Topic::Error);
		eventServer.subscribe(&infoListener, Topic::Info);
		mqttClient.reset();
	}

	TEST_METHOD_INITIALIZE(mqttGatewayMethodInitialize) {
		mqttClient.reset();
		disconnectedListener.reset();
		connectedListener.reset();
		errorListener.reset();
		infoListener.reset();
	}
	TEST_CLASS_CLEANUP(wifiTestClassCleanup) {
		eventServer.unsubscribe(&connectedListener);
		eventServer.unsubscribe(&disconnectedListener);
		eventServer.unsubscribe(&infoListener);
		eventServer.unsubscribe(&errorListener);
	}
	TEST_METHOD(mqttGatewayScriptTest) {
		// We need to make this a longer test since the init needs to be done for the rest to work
		
		// Init part
		MqttGateway gateway(&eventServer, CONFIG_MQTT_BROKER, CONFIG_MQTT_PORT, CONFIG_MQTT_USER, CONFIG_MQTT_PASSWORD);

		gateway.begin(&client, "client1");
		// first check if the connection event was sent (no disconnects, one connect - no more)
		Assert::AreEqual(0, disconnectedListener.getCallCount(), L"no disconnected event");
		Assert::AreEqual(1, connectedListener.getCallCount(), L"connected event");
		Assert::AreEqual(1, errorListener.getCallCount(), L"Error called once");
		Assert::AreEqual("", errorListener.getPayload(), L"Error empty");
		Assert::AreEqual(3, infoListener.getCallCount(), L"Info called three times");
		Assert::AreEqual("MQTT: Announcement complete", infoListener.getPayload(), L"Info last message correct");

		Assert::AreEqual(CONFIG_MQTT_USER, mqttClient.user());
		Assert::AreEqual("client1", mqttClient.id());
		// check if the homie init events were sent 1586 1613
		Assert::AreEqual(static_cast<size_t>(1656), strlen(mqttClient.getTopics()), L"Topic lenght OK");
		Assert::AreEqual(static_cast<size_t>(473), strlen(mqttClient.getPayloads()), L"Payload lenght OK");
		Assert::AreEqual(46, mqttClient.getCallCount());
		mqttClient.reset();

		// Valid callback test
		TestEventClient callBackListener("disconnectedListener", &eventServer);
		eventServer.subscribe(&callBackListener, Topic::BatchSizeDesired);
		char topic[100];
		constexpr int PAYLOAD_SIZE = 2;
		uint8_t payload[PAYLOAD_SIZE] = { '2', '0' };
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
		MqttGateway gateway(&eventServer, CONFIG_MQTT_BROKER, CONFIG_MQTT_PORT, nullptr, "");
		gateway.begin(&client, "client1", false);
		gateway.connect();
		Assert::AreEqual("", mqttClient.user(), "User not set");
	}

	TEST_METHOD(mqttGatewayCannotConnectTest) {
		mqttClient.setCanConnect(false);
	    MqttGateway gateway(&eventServer, CONFIG_MQTT_BROKER, CONFIG_MQTT_PORT, CONFIG_MQTT_USER, CONFIG_MQTT_PASSWORD);
		gateway.begin(&client, "client1");
		Assert::AreEqual(0, disconnectedListener.getCallCount(), L"Disconnected published");
		Assert::AreEqual(0, connectedListener.getCallCount(), L"Connected not published");
		Assert::AreEqual("MQTT: Connecting", infoListener.getPayload(), "tried to connect");
		Assert::AreEqual("MQTT: Could not connect to broker [state = 3]", errorListener.getPayload(), "Error happened");
	}

	TEST_METHOD(mqttGatewayCannotSubscribeTest) {
		mqttClient.setCanSubscribe(false);
		MqttGateway gateway(&eventServer, CONFIG_MQTT_BROKER, CONFIG_MQTT_PORT, CONFIG_MQTT_USER, CONFIG_MQTT_PASSWORD);
		gateway.begin(&client, "client1");
		Assert::AreEqual(1, disconnectedListener.getCallCount(), L"Disconnected published");
		Assert::AreEqual(1, connectedListener.getCallCount(), L"Connected published");
		Assert::AreEqual("MQTT: Connecting", infoListener.getPayload(), "tried to connect");
		Assert::AreEqual("MQTT: Could not subscribe to setters [state = 3]", errorListener.getPayload(), "Error happened");
	}

	TEST_METHOD(mqttGatewayCannotAnnounceTest) {
		mqttClient.setCanPublish(false);
		MqttGateway gateway(&eventServer, CONFIG_MQTT_BROKER, CONFIG_MQTT_PORT, CONFIG_MQTT_USER, CONFIG_MQTT_PASSWORD);
		gateway.begin(&client, "client1");
		Assert::AreEqual(1, disconnectedListener.getCallCount(), L"Disconnected published");
		Assert::AreEqual(1, connectedListener.getCallCount(), L"Connected published");
		Assert::AreEqual("MQTT: Connected and subscribed to setters", infoListener.getPayload(), "tried to connect");
		Assert::AreEqual("MQTT: Could not announce device [state = 3]", errorListener.getPayload(), "Error happened");
	}

	TEST_METHOD(mqttGatewayConnectionLossTest) {
		MqttGateway gateway(&eventServer, CONFIG_MQTT_BROKER, CONFIG_MQTT_PORT, CONFIG_MQTT_USER, CONFIG_MQTT_PASSWORD);
		gateway.begin(&client, "client1");
		Assert::AreEqual(0, disconnectedListener.getCallCount(), L"No disconnect published");
		Assert::AreEqual(1, connectedListener.getCallCount(), L"Connected published");

		mqttClient.setCanConnect(false);
		disconnectedListener.reset();
		connectedListener.reset();
		errorListener.reset();
		// force an evaluation of the connection state
		eventServer.publish(Topic::FreeHeap, 1000);
		Assert::AreEqual("MQTT: Could not connect to broker [state = 3]", errorListener.getPayload(), "Error happened");
		Assert::AreEqual(1, disconnectedListener.getCallCount(), L"Disconnected published");

	    infoListener.reset();
		errorListener.reset();
		// try connecting again
		gateway.handleQueue();
		Assert::AreEqual(0, errorListener.getCallCount(), L"Error not called again");
		Assert::AreEqual(0, infoListener.getCallCount(), L"Info not called again (i.e. no re-init");

		delay(1000);
		gateway.handleQueue();
		Assert::AreEqual("MQTT: Connecting", infoListener.getPayload(), L"Re-init after one second");
	    Assert::AreEqual("MQTT: Could not connect to broker [state = 3]", errorListener.getPayload(), L"Reconnect failed");
		mqttClient.setCanConnect(true);
		connectedListener.reset();
		infoListener.reset();
		errorListener.reset();
		delay(1000);
		gateway.handleQueue();
		Assert::AreEqual(1, connectedListener.getCallCount(), L"reconnected");
		Assert::AreEqual("MQTT: Announcement complete", infoListener.getPayload(), L"Re-initialized");
		Assert::AreEqual("", errorListener.getPayload(), L"Error reset after reconnect");


	}

	};
}
