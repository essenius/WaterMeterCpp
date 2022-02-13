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

// ReSharper disable CppClangTidyClangDiagnosticExitTimeDestructors

#include "pch.h"
#include "CppUnitTest.h"
#ifdef ESP32
#include <ESP.h>
#else
#include "../WaterMeterCpp/ArduinoMock.h"
#endif

#include "../WaterMeterCpp/Device.h"
#include "../WaterMeterCpp/DataQueue.h"
#include "../WaterMeterCpp/FlowMeter.h"
#include "../WaterMeterCpp/LedDriver.h"
#include "../WaterMeterCpp/MagnetoSensorReader.h"
#include "../WaterMeterCpp/MqttGateway.h"
#include "../WaterMeterCpp/EventServer.h"
#include "../WaterMeterCpp/PubSubClientMock.h"
#include "../WaterMeterCpp/ResultAggregator.h"
#include "../WaterMeterCpp/TimeServer.h"
#include "../WaterMeterCpp/Wifi.h"
#include "../WaterMeterCpp/Connector.h"
#include "../WaterMeterCpp/Communicator.h"
#include "../WaterMeterCpp/FirmwareManager.h"
#include "../WaterMeterCpp/Log.h"
#include "../WaterMeterCpp/SampleAggregator.h"
#include "../WaterMeterCpp/QueueClient.h"
#include "../WaterMeterCpp/secrets.h"
#include "../WaterMeterCpp/Sampler.h"
#include "TopicHelper.h"
#include "StateHelper.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// crude mechanism to test the main part -- copy/paste

namespace WaterMeterCppTest {

    TEST_CLASS(MainTest) {
    public:
        TEST_METHOD(mainTest1) {
            // make the firmware check fail
            HTTPClient::ReturnValue = 400;

            // this is needed because other tests might have been running before.
            uxQueueReset();

            // For being able to set the firmware 
            constexpr const auto BUILD_VERSION = "0.100.3";

            // We measure every 10 ms. That is about the fastest that the sensor can do reliably
            // Processing one cycle usually takes quite a bit less than that, unless a write happened.
            constexpr unsigned long MEASURE_INTERVAL_MICROS = 10UL * 1000UL;

            QMC5883LCompass compass;
            EventServer samplerEventServer;
            MagnetoSensorReader sensorReader(&samplerEventServer, &compass);
            FlowMeter flowMeter(&samplerEventServer);

            EventServer communicatorEventServer;
            EventServer connectorEventServer;

            Clock theClock(&communicatorEventServer);
            PayloadBuilder serializePayloadBuilder(&theClock);
            Serializer serializer(&connectorEventServer, &serializePayloadBuilder);
            SensorDataQueuePayload connectorPayload;
            DataQueue sensorDataQueue(&connectorEventServer, &connectorPayload);
            SensorDataQueuePayload measurementPayload;
            SensorDataQueuePayload resultPayload;
            SampleAggregator sampleAggregator(&samplerEventServer, &theClock, &sensorDataQueue, &measurementPayload);
            ResultAggregator resultAggregator(&samplerEventServer, &theClock, &sensorDataQueue, &resultPayload,
                                              MEASURE_INTERVAL_MICROS);

            Device device(&communicatorEventServer);
            LedDriver ledDriver(&communicatorEventServer);
            PayloadBuilder wifiPayloadBuilder;
            Log logger(&communicatorEventServer, &wifiPayloadBuilder);

            Wifi wifi(&connectorEventServer, &WIFI_CONFIG, &wifiPayloadBuilder);
            PubSubClient mqttClient;
            MqttGateway mqttGateway(&connectorEventServer, &mqttClient, &MQTT_CONFIG, &sensorDataQueue, BUILD_VERSION);
            FirmwareManager firmwareManager(&connectorEventServer, CONFIG_BASE_FIRMWARE_URL, BUILD_VERSION);

            QueueClient samplerQueueClient(&samplerEventServer, 20, 0);
            QueueClient communicatorSamplerQueueClient(&communicatorEventServer, 20, 1);
            QueueClient communicatorConnectorQueueClient(&communicatorEventServer, 20, 2);
            QueueClient connectorCommunicatorQueueClient(&connectorEventServer, 100, 3);

            // send only
            QueueClient connectorSamplerQueueClient(&connectorEventServer, 0, 4);
            SensorDataQueuePayload communicatorQueuePayload;
            PayloadBuilder serialize2PayloadBuilder(&theClock);
            Serializer serializer2(&communicatorEventServer, &serialize2PayloadBuilder);

            DataQueue connectorDataQueue(&communicatorEventServer, &communicatorQueuePayload, 1, 1024, 128, 256);
            Sampler sampler(&samplerEventServer, &sensorReader, &flowMeter, &sampleAggregator, &resultAggregator,
                            &samplerQueueClient);
            Communicator communicator(&communicatorEventServer, &logger, &ledDriver, &device, &connectorDataQueue, &serializer2,
                                      &communicatorSamplerQueueClient, &communicatorConnectorQueueClient);

            TimeServer timeServer;
            Connector connector(&connectorEventServer, &wifi, &mqttGateway, &timeServer, &firmwareManager, &sensorDataQueue,
                                &connectorDataQueue, &serializer, &connectorSamplerQueueClient,
                                &connectorCommunicatorQueueClient);

            TaskHandle_t communicatorTaskHandle;
            TaskHandle_t connectorTaskHandle;

            Serial.begin(115200);
            theClock.begin();

            // queue for the sampler process
            samplerQueueClient.begin(communicatorSamplerQueueClient.getQueueHandle());

            // queues for the communicator process
            // receive only
            communicatorSamplerQueueClient.begin();
            communicatorConnectorQueueClient.begin(connectorCommunicatorQueueClient.getQueueHandle());

            // queues for the connector process
            connectorCommunicatorQueueClient.begin(communicatorConnectorQueueClient.getQueueHandle());
            connectorSamplerQueueClient.begin(samplerQueueClient.getQueueHandle());

            // disable the timestamps to make it easier to test
            communicatorEventServer.cannotProvide(&theClock, Topic::Time);

            communicator.setup();
            sampler.setup(MEASURE_INTERVAL_MICROS);
            connector.setup();

            // connect to Wifi, get the time and start the MQTT client. Do this on core 0 (setup and loop run on core 1)
            xTaskCreatePinnedToCore(Connector::task, "Connector", 10000, &connector, 1, &connectorTaskHandle, 0);

            // start the communication task which takes care of logging and leds, as well as passing on data to the connector if there is a connection
            xTaskCreatePinnedToCore(Communicator::task, "Communicator", 10000, &communicator, 1, &communicatorTaskHandle, 0);

            device.begin(xTaskGetCurrentTaskHandle(), communicatorTaskHandle, connectorTaskHandle);

            // begin can only run when both sampler and connector have finished setup, since it can start publishing right away
            sampler.begin();

            // subscribe to a topic normally not subscribed to so we can see if it is dealt with appropriately
            samplerEventServer.subscribe(&logger, Topic::Sample);
            wifi.announceReady();

            sampler.loop();
            communicator.loop();
            connector.loop();
            while (communicatorSamplerQueueClient.receive()) {}
            while (communicatorConnectorQueueClient.receive()) {}

            Assert::AreEqual(Led::OFF, Led::get(Led::AUX), L"AUX still off");
            Assert::AreEqual(Led::ON, Led::get(Led::RUNNING), L"RUNNING on");
            Assert::AreEqual(R"([] Starting
[] Topic '6': 0
[] Wifi summary: {"ssid":"","hostname":"thing1","mac-address":"00:11:22:33:44:55","rssi-dbm":1,"channel":13,"network-id":"192.168.1.0","ip-address":"0.0.0.0","gateway-ip":"0.0.0.0","dns1-ip":"0.0.0.0","dns2-ip":"0.0.0.0","subnet-mask":"255.255.255.0","bssid":"55:44:33:22:11:00"}
[] Free Memory DataQueue #0: 12800
[] Free Heap: 32000
[] Free Stack #0: 1500
[] Free Stack #1: 3750
[] Free Stack #2: 5250
[] Connecting to Wifi
)", Serial.getOutput());
            // reduce the noise in the logger 
            samplerEventServer.unsubscribe(&logger, Topic::Sample);
            communicatorEventServer.unsubscribe(&logger, Topic::Connection);
            // start again with the wifi connection
            WiFi.reset();
            WiFi.connectIn(1);
            while (connector.connect() != ConnectionState::MqttReady) {}
            Serial.println("");
            Assert::AreEqual(ConnectionState::MqttReady, connector.connect(), L"Checking for data");

            // emulate the publication of a result from sensor to log
            SensorDataQueuePayload payload1{};
            payload1.topic = Topic::Result;
            payload1.timestamp = 1000000;
            payload1.buffer.result.sampleCount = 327;
            payload1.buffer.result.smoothAbsDerivativeSmooth = 23.02f;
            sensorDataQueue.send(&payload1);
            Assert::AreEqual(ConnectionState::MqttReady, connector.connect(), L"Reading queue");

            Serial.clearOutput();
            communicator.loop();
            auto expected = R"([] Wifi summary: {"ssid":"essenii30n","hostname":"thing1","mac-address":"00:11:22:33:44:55","rssi-dbm":1,"channel":13,"network-id":"192.168.1.0","ip-address":"10.0.0.2","gateway-ip":"10.0.0.1","dns1-ip":"8.8.8.8","dns2-ip":"8.8.4.4","subnet-mask":"255.255.0.0","bssid":"55:44:33:22:11:00"}
[] Wifi summary: {"ssid":"essenii30n","hostname":"thing1","mac-address":"00:11:22:33:44:55","rssi-dbm":1,"channel":13,"network-id":"192.168.1.0","ip-address":"10.0.0.2","gateway-ip":"10.0.0.1","dns1-ip":"8.8.8.8","dns2-ip":"8.8.4.4","subnet-mask":"255.255.0.0","bssid":"55:44:33:22:11:00"}
[] Error: Firmware version check failed with response code 400. URL:
[] https://nas:8443/static/fota/watermeter/001122334455.version
[] Result: {"timestamp":1970-01-01T00:00:01.000000,"lastValue":0,"summaryCount":{"samples":327,"peaks":0,"flows":0,"maxStreak":0},"exceptionCount":{"outliers":0,"excludes":0,"overruns":0},"duration":{"total":0,"average":0,"max":0},"analysis":{"smoothValue":0,"derivative":0,"smoothDerivative":0,"smoothAbsDerivative":23.02}}
[] Free Stack #0: 1564
)";
            Assert::AreEqual(expected, Serial.getOutput(), L"Formatted result came through");
            Serial.clearOutput();
            // expect an overrun
            delay(10);
            sampler.loop();
            Assert::AreEqual(ConnectionState::MqttReady, connector.loop(), L"Connector loop");
            communicator.loop();
            Assert::AreEqual(0, strncmp("[] Time overrun:", Serial.getOutput(), 16), L"Time overrun");
            connectorEventServer.publish(Topic::ResetSensor, 2);
            sampler.loop();
        }
    };
}
