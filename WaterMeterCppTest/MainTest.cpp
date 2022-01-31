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

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// crude mechanism to test the main part -- copy/paste

namespace WaterMeterCppTest {

    TEST_CLASS(MainTest) {
    public:
        TEST_METHOD(mainTest1) {
            // this is needed because other tests might have been running before.
            uxQueueReset();

            // For being able to set the firmware 
            constexpr const char* const BUILD_VERSION = "0.100.3";

            // We measure every 10 ms. That is about the fastest that the sensor can do reliably
            // Processing one cycle usually takes quite a bit less than that, unless a write happened.
            constexpr unsigned long MEASURE_INTERVAL_MICROS = 10UL * 1000UL;

            QMC5883LCompass compass;
            EventServer samplerEventServer;
            MagnetoSensorReader sensorReader(&samplerEventServer, &compass);
            FlowMeter flowMeter(&samplerEventServer);

            EventServer communicatorEventServer;
            // this is where Clock provides Topic::Time
            Clock theClock(&communicatorEventServer);
            PayloadBuilder serializePayloadBuilder(&theClock);
            Serializer serializer(&serializePayloadBuilder);
            DataQueue dataQueue(&communicatorEventServer, &theClock, &serializer);
            RingbufferPayload measurementPayload{};
            RingbufferPayload resultPayload{};
            SampleAggregator sampleAggregator(&samplerEventServer, &theClock, &dataQueue, &measurementPayload);
            ResultAggregator resultAggregator(&samplerEventServer, &theClock, &dataQueue, &resultPayload, MEASURE_INTERVAL_MICROS);

            LedDriver ledDriver(&communicatorEventServer);
            PayloadBuilder wifiPayloadBuilder;

            Device device(&communicatorEventServer);
            Log logger(&communicatorEventServer, &wifiPayloadBuilder);
            EventServer remoteEventServer;
            Wifi wifi(&remoteEventServer, &WIFI_CONFIG, &wifiPayloadBuilder);
            TimeServer timeServer;
            PubSubClient mqttClient;
            MqttGateway mqttGateway(&remoteEventServer, &mqttClient, &MQTT_CONFIG, &dataQueue, BUILD_VERSION);
            FirmwareManager firmwareManager(&remoteEventServer, CONFIG_BASE_FIRMWARE_URL, BUILD_VERSION);

            QueueClient samplerQueueClient(&samplerEventServer, 20);
            QueueClient communicatorSamplerQueueClient(&communicatorEventServer, 20);
            QueueClient communicatorConnectorQueueClient(&communicatorEventServer, 20);
            QueueClient connectorCommunicatorQueueClient(&remoteEventServer, 100);
            // send only
            QueueClient connectorSamplerQueueClient(&remoteEventServer, 1);

            Sampler sampler(&samplerEventServer, &sensorReader, &flowMeter, &sampleAggregator, &resultAggregator, &samplerQueueClient);
            Communicator communicator(&communicatorEventServer, &logger, &ledDriver, &device, &communicatorSamplerQueueClient, &communicatorConnectorQueueClient);
            Connector connector(&remoteEventServer, &wifi, &mqttGateway, &timeServer, &firmwareManager, &dataQueue, &connectorSamplerQueueClient, &connectorCommunicatorQueueClient);

            TaskHandle_t communicatorTaskHandle;
            TaskHandle_t connectorTaskHandle;

            Serial.begin(115200);
            device.begin(xTaskGetCurrentTaskHandle(), &communicatorTaskHandle, &connectorTaskHandle);
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
[] Free Heap: 32000
[] Free Stack Sampler: 1500
[] Free Stack Communicator: 3750
[] Free Stack Connector: 5250
[] Connecting to Wifi
)", Serial.getOutput());
            samplerEventServer.unsubscribe(&logger, Topic::Sample);

        }
    };
}
