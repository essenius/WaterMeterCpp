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
#include "../WaterMeterCpp/LedDriver.h"
#include "../WaterMeterCpp/MagnetoSensorSimulation.h"
#include "../WaterMeterCpp/MagnetoSensorNull.h"
#include "../WaterMeterCpp/MagnetoSensorReader.h"
#include "../WaterMeterCpp/MqttGateway.h"
#include "../WaterMeterCpp/ResultAggregator.h"
#include "../WaterMeterCpp/TimeServer.h"
#include "../WaterMeterCpp/WiFiManager.h"
#include "../WaterMeterCpp/Log.h"
#include "../WaterMeterCpp/SampleAggregator.h"
#include "../WaterMeterCpp/QueueClient.h"

#include "HTTPClient.h"
#include "SamplerDriver.h"
#include "Wire.h"

// ReSharper restore CppUnusedIncludeDirective

// crude mechanism to test the main part -- copy/paste. We can't do much better than this because we need the
// objects defined globally so we don't get into heap issues.

namespace WaterMeterCppTest {

    class MainDataTest : public testing::Test {
    public:
        static Preferences preferences;
        static Configuration configuration;
    };

    Preferences MainDataTest::preferences;
    Configuration MainDataTest::configuration(&preferences);

    // ReSharper disable once CyclomaticComplexity -- caused by EXPECT macros
    TEST_F(MainDataTest, mainTest1) {
        EXPECT_STREQ("", getPrintOutput()) << "Print buffer empty start";

        setRealTime(false);

        disableDelay(false);
        // make the firmware check fail
        HTTPClient::ReturnValue = 400;
        // Other tests might have run before, so begin stacks and queues
        ESP.restart();
        uxTaskGetStackHighWaterMarkReset();
        uxQueueReset();
        uxRingbufReset();

        // For being able to set the firmware 
        constexpr auto BuildVersion = "0.105.2";

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

        MagnetoSensorSimulation dataSensor;
        MagnetoSensorNull nullSensor;
        MagnetoSensor* sensor[] = { &dataSensor, &nullSensor };

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
        ResultAggregator resultAggregator(&samplerEventServer, &theClock, &sensorDataQueue, &resultPayload, MeasureIntervalMicros);

        Device device(&communicatorEventServer);
        Meter meter(&communicatorEventServer);
        LedDriver ledDriver(&communicatorEventServer);
        OledDriver oledDriver(&communicatorEventServer, &Wire1);
        PayloadBuilder wifiPayloadBuilder;
        Log logger(&communicatorEventServer, &wifiPayloadBuilder);

        WiFiManager wifi(&connectorEventServer, &configuration.wifi, &wifiPayloadBuilder);
        PubSubClient mqttClient;
        MqttGateway mqttGateway(&connectorEventServer, &mqttClient, &wifiClientFactory, &configuration.mqtt, &sensorDataQueue,
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

        Serial.begin(230400);
        theClock.begin();
        // ReSharper disable once CppUseStdSize -- we need a C++ 11 compatible way
        sensorReader.begin(sensor, sizeof sensor / sizeof sensor[0]);

        Wire1.begin(SdaOled, SclOled);

        FirmwareConfig firmwareConfig{ "https://localhost/" };
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

        // this is where setup ends

        while (!dataSensor.done()) {
            // emulate a timer trigger
            SamplerDriver::onTimer();
            sampler.sensorLoop();
            sampler.loop();
            communicator.loop();
            connector.loop();
            while (communicatorSamplerQueueClient.receive()) {}
            while (communicatorConnectorQueueClient.receive()) {}
        }
        printf(getPrintOutput());
        clearPrintOutput();
    }
}
