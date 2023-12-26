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

#include "../WaterMeter/Button.h"
#include "../WaterMeter/Communicator.h"
#include "../WaterMeter/Connector.h"
#include "../WaterMeter/Device.h"
#include "../WaterMeter/EventServer.h"
#include "../WaterMeter/FirmwareManager.h"
#include "../WaterMeter/DataQueue.h"
#include "../WaterMeter/Led.h"
#include "../WaterMeter/LedDriver.h"
#include "../WaterMeter/MagnetoSensorHmc.h"
#include "../WaterMeter/MagnetoSensorQmc.h"
#include "../WaterMeter/MagnetoSensorNull.h"
#include "../WaterMeter/MagnetoSensorReader.h"
#include "../WaterMeter/MqttGateway.h"
#include "../WaterMeter/ResultAggregator.h"
#include "../WaterMeter/TimeServer.h"
#include "../WaterMeter/WiFiManager.h"
#include "../WaterMeter/Log.h"
#include "../WaterMeter/SampleAggregator.h"
#include "../WaterMeter/QueueClient.h"

#include "HTTPClient.h"
#include "SamplerDriver.h"
#include "TestEventClient.h"
#include "WiFi.h"
#include "Wire.h"

// ReSharper restore CppUnusedIncludeDirective

// crude mechanism to test the main part -- copy/paste. We can't do much better than this because we need the
// objects defined globally so we don't get into heap issues.

namespace WaterMeterCppTest {
    using EllipseMath::EllipseFit;
    using namespace WaterMeter;

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
            Wire.setFlatline(true, 0);
            Wire.setEndTransmissionTogglePeriod(10);
            uxTaskGetStackHighWaterMarkReset();
            uxQueueReset();
            uxRingbufReset();

            // For being able to set the firmware 
            constexpr auto BuildVersion = "0.103.0";

            // We measure every 10 ms. That is twice the frequency of the AC in Europe, which we need to take into account since
            // there are water pumps close to the water meter, and is about the fastest that the sensor can do reliably.
            // Processing one cycle usually takes quite a bit less than that.

            constexpr unsigned long MeasureIntervalMicros = 10UL * 1000UL;

            constexpr int SdaOled = 32;
            constexpr int SclOled = 33;
            constexpr int ButtonPort = 34;

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
                                              MeasureIntervalMicros);

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
                                    BuildVersion);
            FirmwareManager firmwareManager(&connectorEventServer, &wifiClientFactory, &configuration.firmware, BuildVersion);

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
            Button button(&buttonPublisher, ButtonPort);

            DataQueue connectorDataQueue(&connectorEventServer, &connectorDataQueuePayload, 1, 1024, 128, 256);
            SamplerDriver sampler(&samplerEventServer, &sensorReader, &flowDetector, &button, &sampleAggregator, &resultAggregator,
                            &samplerQueueClient);
            Communicator communicator(&communicatorEventServer, &oledDriver, &device,
                                      &connectorDataQueue, &serializer2,
                                      &communicatorSamplerQueueClient, &communicatorConnectorQueueClient);

            TimeServer timeServer;
            Connector connector(&connectorEventServer, &wifi, &mqttGateway, &timeServer, &firmwareManager, &sensorDataQueue,
                                &connectorDataQueue, &serializer, &connectorSamplerQueueClient,
                                &connectorCommunicatorQueueClient);

            static constexpr BaseType_t Core1 = 1;
            static constexpr BaseType_t Core0 = 0;
            static constexpr uint16_t StackDepth = 10000;
            static constexpr BaseType_t Priority1 = 1;

            TaskHandle_t samplerTaskHandle;
            TaskHandle_t communicatorTaskHandle;
            TaskHandle_t connectorTaskHandle;

            clearPrintOutput();

            Serial.begin(115200);
            theClock.begin();
            // ReSharper disable once CppUseStdSize -- we need a C++ 11 compatible way
            sensorReader.begin(sensor, sizeof sensor / sizeof sensor[0]);

            Wire.begin(); // standard SDA=21, SCL=22
            Wire.setFlatline(true, 0);
            Wire1.begin(SdaOled, SclOled);

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

            EXPECT_TRUE(sampler.begin(sensor, std::size(sensor), MeasureIntervalMicros)) << "Sampler found a sensor";
            EXPECT_STREQ("[] Starting\n", getPrintOutput()) << "Print output started";
            clearPrintOutput();

            // On timer fire, read from the sensor and put sample in a queue. Use core 1 so we are not (or at least much less) influenced by Wifi and printing
            xTaskCreatePinnedToCore(Sampler::task, "Sampler", StackDepth, &sampler, Priority1, &samplerTaskHandle, Core1);

            // connect to Wifi, get the time and start the MQTT client. Do this on core 0 (where WiFi runs as well)
            xTaskCreatePinnedToCore(Connector::task, "Connector", StackDepth, &connector, Priority1, &connectorTaskHandle, Core0);

            // Take care of logging and leds, as well as passing on data to the connector if there is a connection. Also on core 0, as not time sensitive
            xTaskCreatePinnedToCore(Communicator::task, "Communicator", StackDepth, &communicator, Priority1, &communicatorTaskHandle, Core0);

            // beginLoop can only run when both sampler and connector have finished settting up, since they can start publishing right away.
            // This also starts the hardware timer.
            sampler.beginLoop(samplerTaskHandle);

            device.begin(xTaskGetCurrentTaskHandle(), communicatorTaskHandle, connectorTaskHandle);

            // subscribe to a topic normally not subscribed to so we can see if it is dealt with appropriately
            samplerEventServer.subscribe(&logger, Topic::Sample);
            wifi.announceReady();

            // emulate a timer trigger
            SamplerDriver::onTimer();
            sampler.sensorLoop();
            sampler.loop();
            communicator.loop();
            connector.loop();
            while (communicatorSamplerQueueClient.receive()) {}
            while (communicatorConnectorQueueClient.receive()) {}

            EXPECT_EQ(Led::On, Led::get(Led::Aux)) << "AUX on";
            EXPECT_EQ(Led::On, Led::get(Led::Running)) << "RUNNING on";
            EXPECT_STREQ(R"([] Topic '6': 0
[] Free Spaces Queue #1: 18
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
            payload1.buffer.result.ellipseCenterTimes10 = {{-10, -553}};
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
[] Result: {"timestamp":1970-01-01T00:00:01.000000,"last.x":0,"last.y":0,"summaryCount":{"samples":327,"pulses":0,"maxStreak":0,"skips":34},"exceptionCount":{"outliers":0,"overruns":0,"resets":0},"duration":{"total":0,"average":0,"max":0},"ellipse":{"cx":-1,"cy":-55.3,"rx":0,"ry":0,"phi":0}}
[] Free Stack #0: 1564
)";
            EXPECT_STREQ(expected, getPrintOutput()) << "Formatted result came through";
            clearPrintOutput();

            // force an overrun 

            SamplerDriver::onTimer();
            delay(25);
            sampler.sensorLoop();
            sampler.loop();

            // switch off delay() so we don't get an overrun next time

            //disableDelay(true);

            EXPECT_EQ(ConnectionState::MqttReady, connector.loop()) << "Connector loop";
            communicator.loop();
            EXPECT_STREQ("[] Time overrun: 1140650\n[] Free Stack #0: 1628\n", getPrintOutput()) << "Time overrun";

            connectorEventServer.publish(Topic::ResetSensor, 2);
            TestEventClient client(&samplerEventServer);
            samplerEventServer.subscribe(&client, Topic::SensorWasReset);
            SamplerDriver::onTimer();
            sampler.sensorLoop();
            sampler.loop();
            EXPECT_EQ(1, client.getCallCount()) << "SensorWasReset was called";

            clearPrintOutput();
            communicator.loop();

            auto expected2 = R"([] Time overrun: 65250
[] Free Spaces Queue #1: 14
[] Sensor state: 0 (0 = none, 1 = ok)
[] Sensor state: 3 (0 = none, 1 = ok)
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
