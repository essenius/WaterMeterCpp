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

#include "gtest/gtest.h"
#include "TestEventClient.h"
#include <PubSubClient.h>
#include "../WaterMeterCpp/MqttGateway.h"
#include "../WaterMeterCpp/EventServer.h"
#include "../WaterMeterCpp/PayloadBuilder.h"
#include "../WaterMeterCpp/DataQueue.h"
#include "../WaterMeterCpp/SafeCString.h"
#include "../WaterMeterCpp/Serializer.h"

namespace WaterMeterCppTest {

    constexpr MqttConfig MQTT_CONFIG_WITH_USER{"broker", 1883, "user", "password", false};
    constexpr MqttConfig MQTT_CONFIG_NO_USER{"broker", 1883, nullptr, "", false};

    constexpr const char* const BUILD = "1";

    class MqttGatewayTest : public testing::Test {
    public:
        static Client client;

        static EventServer eventServer;
        static Clock theClock;
        static TestEventClient errorListener;
        static TestEventClient infoListener;
        static PubSubClient mqttClient;
        static PayloadBuilder payloadBuilder;
        static Serializer serializer;
        static DataQueuePayload payload;
        static DataQueue dataQueue;

        // ReSharper disable once CppInconsistentNaming
        static void SetUpTestCase() {
            eventServer.subscribe(&errorListener, Topic::ConnectionError);
            eventServer.subscribe(&infoListener, Topic::Info);
            mqttClient.reset();
        }

        void SetUp() override {
            mqttClient.reset();
            errorListener.reset();
            infoListener.reset();
        }

        void TearDown() override {
            eventServer.unsubscribe(&infoListener);
            eventServer.unsubscribe(&errorListener);
        }
    };

    Client MqttGatewayTest::client;
    EventServer MqttGatewayTest::eventServer;
    Clock MqttGatewayTest::theClock(&eventServer);
    TestEventClient MqttGatewayTest::errorListener(&eventServer);
    TestEventClient MqttGatewayTest::infoListener(&eventServer);
    PubSubClient MqttGatewayTest::mqttClient;
    PayloadBuilder MqttGatewayTest::payloadBuilder;
    DataQueuePayload MqttGatewayTest::payload;
    DataQueue MqttGatewayTest::dataQueue(&eventServer, &payload);

    TEST_F(MqttGatewayTest, mqttGatewayCannotSubscribeTest) {
        mqttClient.setCanSubscribe(false);
        WiFiClientFactory wifiClientFactory(nullptr);
        MqttGateway gateway(&eventServer, &mqttClient, &wifiClientFactory, &MQTT_CONFIG_WITH_USER, &dataQueue, BUILD);
        gateway.begin("client1");
        EXPECT_STREQ("MQTT: Could not subscribe to setters [state = 3]", errorListener.getPayload()) << "Error happened";
    }

    TEST_F(MqttGatewayTest, mqttGatewayNoUserTest) {
        WiFiClientFactory wifiClientFactory(nullptr);
        MqttGateway gateway(&eventServer, &mqttClient, &wifiClientFactory, &MQTT_CONFIG_NO_USER, &dataQueue, BUILD);
        gateway.begin("client1");
        EXPECT_STREQ("", mqttClient.user()) << "User not set";
    }

    // ReSharper disable once CyclomaticComplexity -- caused by EXPECT macros
    TEST_F(MqttGatewayTest, mqttGatewayScriptTest) {
        // We need to make this a longer test since the init needs to be done for the rest to work
        WiFiClientFactory wifiClientFactory(nullptr);

        // Init part
        MqttGateway gateway(&eventServer, &mqttClient, &wifiClientFactory, &MQTT_CONFIG_WITH_USER, &dataQueue, BUILD);

        gateway.begin("client1");

        TestEventClient volumeListener(&eventServer);
        eventServer.subscribe(&volumeListener, Topic::AddVolume);

        constexpr int LOOP_PAYLOAD_SIZE = 7;
        uint8_t loopPayload[LOOP_PAYLOAD_SIZE] = { '1','2','3','.','4','5','6' };
        mqttClient.setLoopCallback("homie/client1/result/meter", loopPayload, LOOP_PAYLOAD_SIZE);

        EXPECT_TRUE(gateway.getPreviousVolume()) << "Found previous volume";
        ASSERT_EQ(1, volumeListener.getCallCount()) << "Volume published";
        ASSERT_STREQ("123.456", volumeListener.getPayload()) << "volume payload ok";
        EXPECT_FALSE(gateway.getPreviousVolume()) << "Previous volume only works once";
        mqttClient.setLoopCallback("\0", {}, 0);

        int count = 0;
        while (gateway.hasAnnouncement()) {
            EXPECT_TRUE(gateway.publishNextAnnouncement()) << "Announcement #" << count;
            count++;
        }
        EXPECT_EQ(58, count) << "announcement count";
        gateway.publishNextAnnouncement();
        EXPECT_EQ(0, errorListener.getCallCount()) << "Error not called";
        EXPECT_EQ(0, infoListener.getCallCount()) << "Info not called";

        EXPECT_STREQ(MQTT_CONFIG_WITH_USER.user, mqttClient.user()) << "User OK";
        EXPECT_STREQ("client1", mqttClient.id()) << "Client ID OK";
        // check if the homie init events were sent 
        EXPECT_EQ(static_cast<size_t>(2193), strlen(mqttClient.getTopics())) << "Topic lenght OK";
        EXPECT_EQ(static_cast<size_t>(627), strlen(mqttClient.getPayloads())) << "Payload lenght OK";
        EXPECT_EQ(58, mqttClient.getCallCount()) << "Call count";

        gateway.announceReady();

        mqttClient.reset();

        // Incoming event from event server should get published

        eventServer.publish(Topic::Rate, 7);
        EXPECT_STREQ("homie/client1/result/rate\n", mqttClient.getTopics()) << "Payload OK";
        EXPECT_STREQ("7\n", mqttClient.getPayloads()) << "Payload OK";

        mqttClient.reset();

        // a topic that shouldn't be retained

        eventServer.publish(Topic::SensorWasReset, 1);
        EXPECT_STREQ("homie/client1/device/reset-sensor\n", mqttClient.getTopics()) << "Payload OK";
        EXPECT_STREQ("1[x]\n", mqttClient.getPayloads()) << "Payload OK";

        // Incoming valid callback from MQTT should get passed on to the event server

        TestEventClient callBackListener(&eventServer);
        eventServer.subscribe(&callBackListener, Topic::BatchSizeDesired);

        char topic[100];
        constexpr int PAYLOAD_SIZE = 2;
        uint8_t payload1[PAYLOAD_SIZE] = {'2', '0'};
        safeStrcpy(topic, "homie/device_id/measurement/batch-size-desired/set");
        mqttClient.callBack(topic, payload1, PAYLOAD_SIZE);
        EXPECT_EQ(1, callBackListener.getCallCount()) << "callBackListener called";
        EXPECT_STREQ("20", callBackListener.getPayload()) << "callBackListener got right payload";
        callBackListener.reset();

        // Meter setup
        eventServer.subscribe(&callBackListener, Topic::SetVolume);

        safeStrcpy(topic, "homie/device_id/result/meter/set");
        mqttClient.callBack(topic, payload1, PAYLOAD_SIZE);
        EXPECT_EQ(1, callBackListener.getCallCount()) << "callBackListener called";
        EXPECT_STREQ("20", callBackListener.getPayload()) << "callBackListener got right payload";
        callBackListener.reset();

        // Empty payload should get ignored
        mqttClient.callBack(topic, payload1, 0);
        EXPECT_EQ(0, callBackListener.getCallCount()) << "callBackListener not called";
        callBackListener.reset();

        // Invalid callback should get ignored

        callBackListener.reset();
        safeStrcpy(topic, "homie/device_id/measurement/batch-size-desired/get");
        mqttClient.callBack(topic, payload1, PAYLOAD_SIZE);
        EXPECT_EQ(0, callBackListener.getCallCount()) << "callBackListener not called";
        callBackListener.reset();

        // Empty topic should get ignored, just checking nothing breaks

        topic[0] = 0;
        mqttClient.callBack(topic, payload1, PAYLOAD_SIZE);

        // null pointer as topic should get ignored 

        mqttClient.callBack(nullptr, payload1, PAYLOAD_SIZE);

        // same with a payload not having a device id

        safeStrcpy(topic, "bogus");
        mqttClient.callBack(topic, payload1, PAYLOAD_SIZE);

        // same with a payload not having a node or a property

        safeStrcpy(topic, "homie/device");
        mqttClient.callBack(topic, payload1, PAYLOAD_SIZE);

        // a topic we don't know should be ignored

        safeStrcpy(topic, "homie/device_id/bogus/batch-size-desired/set");
        mqttClient.callBack(topic, payload1, PAYLOAD_SIZE);
        EXPECT_EQ(0, callBackListener.getCallCount()) << "callBackListener not called";

        gateway.handleQueue();
        EXPECT_EQ(1, mqttClient.getLoopCount()) << "Loop count 1";
        mqttClient.setCanConnect(false);
        gateway.handleQueue();
        EXPECT_EQ(1, mqttClient.getLoopCount()) << "Loop count still 1 after disconnect";

        mqttClient.reset();

        eventServer.publish(Topic::Alert, LONG_TRUE);
        EXPECT_STREQ("homie/client1/$state\n", mqttClient.getTopics()) << "Payload OK";
        EXPECT_STREQ("alert\n", mqttClient.getPayloads()) << "Payload OK";
    }
}
