// Copyright 2021-2023 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

// ReSharper disable CppClangTidyClangDiagnosticExitTimeDestructors

#include "gtest/gtest.h"

#include <ESP.h>
#include <PubSubClient.h>

#include "../WaterMeterCpp/Button.h"
#include "../WaterMeterCpp/Communicator.h"
#include "../WaterMeterCpp/Connector.h"
#include "../WaterMeterCpp/Device.h"
#include "../WaterMeterCpp/EventServer.h"
#include "../WaterMeterCpp/FirmwareManager.h"
#include "../WaterMeterCpp/DataQueue.h"
#include "../WaterMeterCpp/Led.h"
#include "../WaterMeterCpp/LedDriver.h"
#include "../WaterMeterCpp/MagnetoSensorHmc.h"
#include "../WaterMeterCpp/MagnetoSensorQmc.h"
#include "../WaterMeterCpp/MagnetoSensorNull.h"
#include "../WaterMeterCpp/MagnetoSensorReader.h"
#include "../WaterMeterCpp/MqttGateway.h"
#include "../WaterMeterCpp/ResultAggregator.h"
#include "../WaterMeterCpp/TimeServer.h"
#include "../WaterMeterCpp/WiFiManager.h"
#include "../WaterMeterCpp/Log.h"
#include "../WaterMeterCpp/SampleAggregator.h"
#include "../WaterMeterCpp/QueueClient.h"
#include "../WaterMeterCpp/Sampler.h"

#include "HTTPClient.h"
#include "TestEventClient.h"
#include "WiFi.h"
#include "Wire.h"

// ReSharper restore CppUnusedIncludeDirective

// crude mechanism to test the main part -- copy/paste. We can't do much better than this because we need the
// objects defined globally so we don't get into heap issues.

namespace WaterMeterCppTest {
    
    class MainTest : public testing::Test {
    public:
        static Preferences preferences;
        static Configuration configuration;
    };

    Preferences MainTest::preferences;
    Configuration MainTest::configuration(&preferences);

    // ReSharper disable once CyclomaticComplexity -- caused by EXPECT macros
    TEST_F(MainTest, mainTest1) {
            EXPECT_STREQ("", getPrintOutput()) << "Print buffer empty start";

            setRealTime(false);
            disableDelay(false);
            // make the firmware check fail
            HTTPClient::ReturnValue = 400;
            // Other tests might have run before, so begin stacks and queues
            ESP.restart();
            Wire.setFlatline(true);
            Wire.setEndTransmissionTogglePeriod(10);
            uxTaskGetStackHighWaterMarkReset();
            uxQueueReset();
            uxRingbufReset();

            // For being able to set the firmware 
            constexpr auto BUILD_VERSION = "0.102.1";

            // We measure every 10 ms. That is twice the frequency of the AC in Europe, which we need to take into account since
            // there are water pumps close to the water meter, and is about the fastest that the sensor can do reliably.
            // Processing one cycle usually takes quite a bit less than that.

            constexpr unsigned long MEASURE_INTERVAL_MICROS = 10UL * 1000UL;

            constexpr int SDA_OLED = 32;
            constexpr int SCL_OLED = 33;
            constexpr int BUTTON_PORT = 34;

            // This is where you would normally use an injector framework,
            // We define the objects globally to avoid using (and fragmenting) the heap.
            // we do use dependency injection to hide this design decision as much as possible
            // (and make testing easier).

            MagnetoSensorQmc qmcSensor(&Wire);
            MagnetoSensorHmc hmcSensor(&Wire);
            MagnetoSensorNull nullSensor;
            MagnetoSensor* sensor[] = {&qmcSensor, &hmcSensor, &nullSensor};

            WiFiClientFactory wifiClientFactory(&configuration.tls);
            EventServer samplerEventServer;
            MagnetoSensorReader sensorReader(&samplerEventServer);
            EllipseFit ellipseFit;
            FlowDetector flowDetector(&samplerEventServer, &ellipseFit);

            EventServer communicatorEventServer;
            EventServer connectorEventServer;

            Clock theClock(&communicatorEventServer);
            PayloadBuilder serializePayloadBuilder(&theClock);
            Serializer serializer(&connectorEventServer, &serializePayloadBuilder);
            DataQueuePayload connectorPayload;
            DataQueue sensorDataQueue(&connectorEventServer, &connectorPayload);
            DataQueuePayload measurementPayload;
            DataQueuePayload resultPayload;
            SampleAggregator sampleAggregator(&samplerEventServer, &theClock, &sensorDataQueue, &measurementPayload);
            ResultAggregator resultAggregator(&samplerEventServer, &theClock, &sensorDataQueue, &resultPayload,
                                              MEASURE_INTERVAL_MICROS);

            Device device(&communicatorEventServer);
            Meter meter(&communicatorEventServer);
            LedDriver ledDriver(&communicatorEventServer);
            OledDriver oledDriver(&communicatorEventServer, &Wire1);
            PayloadBuilder wifiPayloadBuilder;
            Log logger(&communicatorEventServer, &wifiPayloadBuilder);

            WiFiManager wifi(&connectorEventServer, &configuration.wifi, &wifiPayloadBuilder);
            PubSubClient mqttClient;
            MqttGateway mqttGateway(&connectorEventServer, &mqttClient, &wifiClientFactory, &configuration.mqtt,
                                    &sensorDataQueue,
                                    BUILD_VERSION);
            FirmwareManager firmwareManager(&connectorEventServer, &wifiClientFactory, &configuration.firmware, BUILD_VERSION);

            QueueClient samplerQueueClient(&samplerEventServer, &logger, 50, 0);
            // will fill fast if we have flow during startup
            QueueClient communicatorSamplerQueueClient(&communicatorEventServer, &logger, 100, 1);
            QueueClient communicatorConnectorQueueClient(&communicatorEventServer, &logger, 50, 2);
            // This queue needs more space as it won't be read when offline.
            QueueClient connectorCommunicatorQueueClient(&connectorEventServer, &logger, 100, 3);

            // Nothing to send from sampler to connector
            QueueClient connectorSamplerQueueClient(&connectorEventServer, &logger, 0, 4);
            DataQueuePayload connectorDataQueuePayload;
            PayloadBuilder serialize2PayloadBuilder(&theClock);
            Serializer serializer2(&communicatorEventServer, &serialize2PayloadBuilder);

            // we need the button in the sampler loop and digitalRead() is fast enough, so we don't use the detour via Communicator here
            ChangePublisher<uint8_t> buttonPublisher(&samplerEventServer, Topic::ResetSensor);
            Button button(&buttonPublisher, BUTTON_PORT);

            DataQueue connectorDataQueue(&connectorEventServer, &connectorDataQueuePayload, 1, 1024, 128, 256);
            Sampler sampler(&samplerEventServer, &sensorReader, &flowDetector, &button, &sampleAggregator, &resultAggregator,
                            &samplerQueueClient);
            Communicator communicator(&communicatorEventServer, &oledDriver, &device,
                                      &connectorDataQueue, &serializer2,
                                      &communicatorSamplerQueueClient, &communicatorConnectorQueueClient);

            TimeServer timeServer;
            Connector connector(&connectorEventServer, &wifi, &mqttGateway, &timeServer, &firmwareManager, &sensorDataQueue,
                                &connectorDataQueue, &serializer, &connectorSamplerQueueClient,
                                &connectorCommunicatorQueueClient);

            TaskHandle_t communicatorTaskHandle;
            TaskHandle_t connectorTaskHandle;

            clearPrintOutput();

            Serial.begin(115200);
            theClock.begin();
            sensorReader.power(HIGH);

            // wait for the sensor to be ready for measurements
            delay(50);
            Wire.begin(); // standard SDA=21, SCL=22
            Wire1.begin(SDA_OLED, SCL_OLED);

            FirmwareConfig firmwareConfig{"https://localhost/"};
            configuration.putFirmwareConfig(&firmwareConfig);
            configuration.begin(false);

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
            EXPECT_STREQ("", getPrintOutput()) << "Print output empty 1";
            communicator.begin();
            connector.begin(&configuration);

            EXPECT_TRUE(sampler.begin(sensor, std::size(sensor), MEASURE_INTERVAL_MICROS)) << "Sampler found a sensor";
            EXPECT_STREQ("[] Starting\n", getPrintOutput()) << "Print output started";
            clearPrintOutput();

            // connect to Wifi, get the time and start the MQTT client. Do this on core 0 (where WiFi runs as well)
            xTaskCreatePinnedToCore(Connector::task, "Connector", 10000, &connector, 1, &connectorTaskHandle, 0);

            // the communicator loop takes care of logging and leds, as well as passing on data to the connector if there is a connection
            xTaskCreatePinnedToCore(Communicator::task, "Communicator", 10000, &communicator, 1, &communicatorTaskHandle, 0);

            EXPECT_STREQ("", getPrintOutput()) << "Print output empty 2";

            // beginLoop can only run when both sampler and connector have finished setup, since it can start publishing right away
            sampler.beginLoop();

            device.begin(xTaskGetCurrentTaskHandle(), communicatorTaskHandle, connectorTaskHandle);

            // subscribe to a topic normally not subscribed to so we can see if it is dealt with appropriately
            samplerEventServer.subscribe(&logger, Topic::Sample);
            wifi.announceReady();

            sampler.loop();
            communicator.loop();
            connector.loop();
            while (communicatorSamplerQueueClient.receive()) {}
            while (communicatorConnectorQueueClient.receive()) {}

            EXPECT_EQ(Led::ON, Led::get(Led::AUX)) << "AUX on";
            EXPECT_EQ(Led::ON, Led::get(Led::RUNNING)) << "RUNNING on";
            EXPECT_STREQ(R"([] Topic '6': 0
[] Free Spaces Queue #1: 19
[] Wifi summary: {"ssid":"","hostname":"","mac-address":"00:11:22:33:44:55","rssi-dbm":1,"channel":13,"network-id":"192.168.1.0","ip-address":"0.0.0.0","gateway-ip":"0.0.0.0","dns1-ip":"0.0.0.0","dns2-ip":"0.0.0.0","subnet-mask":"255.255.255.0","bssid":"55:44:33:22:11:00"}
[] Free Spaces Queue #2: 19
[] Free Memory DataQueue #0: 12800
[] Free Heap: 32000
[] Free Stack #0: 1500
[] Free Stack #1: 3750
[] Free Stack #2: 5250
[] Connecting to Wifi
)", getPrintOutput()) << "first log OK";
            // reduce the noise in the logger 
            samplerEventServer.unsubscribe(&logger, Topic::Sample);
            communicatorEventServer.unsubscribe(&logger, Topic::Connection);
            clearPrintOutput();
            // start again with the wifi connection
            WiFi.reset();
            WiFi.connectIn(1);
            int i = 0;
            // connect 
            while (connector.connect() != ConnectionState::MqttReady) {
                i++;
                if (i > 20) {
                    EXPECT_STREQ("", getPrintOutput()) << "waited > 20 times";
                }
            }
            clearPrintOutput();

            // run a queue read cycle, which should fire a FreeQueueSize event (which hasn't yet been sent to Communicator)
            EXPECT_EQ(ConnectionState::MqttReady, connector.connect()) << "Checking for data";

            // emulate the publication of a result from sensor to log
            DataQueuePayload payload1{};
            payload1.topic = Topic::Result;
            payload1.timestamp = 1000000;
            payload1.buffer.result.sampleCount = 327;
            payload1.buffer.result.skipCount = 34;
            payload1.buffer.result.averaged = { -1.0, -55.3 };
            sensorDataQueue.send(&payload1);
            EXPECT_EQ(ConnectionState::MqttReady, connector.connect()) << "Reading queue";

            communicator.loop();
            auto expected = R"([] Free Spaces Queue #2: 6
[] Wifi summary: {"ssid":"","hostname":"esp32_001122334455","mac-address":"00:11:22:33:44:55","rssi-dbm":1,"channel":13,"network-id":"192.168.1.0","ip-address":"10.0.0.2","gateway-ip":"10.0.0.1","dns1-ip":"8.8.8.8","dns2-ip":"8.8.4.4","subnet-mask":"255.255.0.0","bssid":"55:44:33:22:11:00"}
[] Wifi summary: {"ssid":"","hostname":"esp32_001122334455","mac-address":"00:11:22:33:44:55","rssi-dbm":1,"channel":13,"network-id":"192.168.1.0","ip-address":"10.0.0.2","gateway-ip":"10.0.0.1","dns1-ip":"8.8.8.8","dns2-ip":"8.8.4.4","subnet-mask":"255.255.0.0","bssid":"55:44:33:22:11:00"}
[] Free Spaces Queue #2: 11
[] Free Memory DataQueue #1: 12800
[] Free Spaces Queue #2: 16
[] Free Spaces Queue #3: 16
[] Free Memory DataQueue #1: 12544
[] Error: Firmware version check failed with response code 400. URL:
[] https://localhost/001122334455.version
[] Result: {"timestamp":1970-01-01T00:00:01.000000,"last.x":0,"last.y":0,"summaryCount":{"samples":327,"pulses":0,"maxStreak":0,"skips":34},"exceptionCount":{"outliers":0,"overruns":0,"resets":0},"duration":{"total":0,"average":0,"max":0},"analysis":{"av.x":-1,"av.y":-55.3}}
[] Free Stack #0: 1564
)";
            EXPECT_STREQ(expected, getPrintOutput()) << "Formatted result came through";
            clearPrintOutput();

            // expect an overrun due to delays in the loop tasks

            sampler.loop();

            // switch off delay() so we don't get an overrun next time

            disableDelay(true);

            EXPECT_EQ(ConnectionState::MqttReady, connector.loop()) << "Connector loop";
            communicator.loop();
            EXPECT_STREQ("[] Time overrun: 1095500\n[] Skipped 110 samples\n[] Free Stack #0: 1628\n", getPrintOutput()) << "Time overrun";

            connectorEventServer.publish(Topic::ResetSensor, 2);
            TestEventClient client(&samplerEventServer);
            samplerEventServer.subscribe(&client, Topic::SensorWasReset);
            sampler.loop();
            EXPECT_EQ(1, client.getCallCount()) << "SensorWasReset was called";

            clearPrintOutput();
            communicator.loop();

            auto expected2 = R"([] No sensor found: 1
[] Alert: 1
[] Sensor was reset: 2
[] Free Spaces Queue #0: 20
[] Free Heap: 23000
)";
            EXPECT_STREQ(expected2, getPrintOutput()) << "Communicator picked up alert and begin";

            clearPrintOutput();
            connectorEventServer.publish(Topic::SetVolume, "98765.4321098");
            communicator.loop();
            EXPECT_STREQ("[] Set meter volume: 98765.4321098\n", getPrintOutput()) << "Set meter worked";

            disableDelay(false);
            clearPrintOutput();
        }
    }
