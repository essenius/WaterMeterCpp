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
#include "../WaterMeterCpp/FirmwareManager.h"
#include "../WaterMeterCpp/Log.h"
#include "../WaterMeterCpp/SampleAggregator.h"
#include "../WaterMeterCpp/QueueClient.h"
#include "../WaterMeterCpp/secrets.h" // for all CONFIG constants
#include "../WaterMeterCpp/Sampler.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {

// TODO: this main part is way too big. Fix that.

// For being able to set the firmware 
constexpr const char* const BUILD_VERSION = "0.100.1";

// We measure every 10 ms. That is about the fastest that the sensor can do reliably
// Processing one cycle takes about 8ms max, so that is also within the limit.
constexpr unsigned long MEASURE_INTERVAL_MICROS = 10UL * 1000UL;
constexpr signed long MIN_MICROS_FOR_CHECKS = MEASURE_INTERVAL_MICROS / 5L;

    TEST_CLASS(MainTest) {
    public:
        TEST_METHOD(mainTest1) {
            // this is needed because other tests might have been running before.
            uxQueueReset();
            EventServer samplerEventServer;
            EventServer communicationEventServer;
            LedDriver ledDriver(&communicationEventServer);
            Wifi wifi(&communicationEventServer, &WIFI_CONFIG);
            TimeServer timeServer(&communicationEventServer);
            Device device(&samplerEventServer);
            MagnetoSensorReader sensorReader(&samplerEventServer);
            FlowMeter flowMeter(&samplerEventServer);
            RingbufferPayload measurementPayload{};
            PayloadBuilder payloadBuilder;
            Serializer serializer(&payloadBuilder);
            DataQueue dataQueue(&communicationEventServer, &serializer);
            RingbufferPayload resultPayload{};
            Log logger(&communicationEventServer);
            PubSubClient mqttClient;
            MqttGateway mqttGateway(&communicationEventServer, &mqttClient, &MQTT_CONFIG, &dataQueue, BUILD_VERSION);
            FirmwareManager firmwareManager(&communicationEventServer, CONFIG_BASE_FIRMWARE_URL, BUILD_VERSION);

            SampleAggregator sampleAggregator(&samplerEventServer, &dataQueue, &measurementPayload);
            ResultAggregator resultAggregator(&samplerEventServer, &dataQueue, &resultPayload, MEASURE_INTERVAL_MICROS);


            QueueClient samplerQueueClient(&samplerEventServer);
            QueueClient communicationQueueClient(&communicationEventServer);

            Sampler sampler(&samplerEventServer, &sensorReader, &flowMeter, &sampleAggregator, &resultAggregator, &device, &samplerQueueClient);
            Connector connection(&communicationEventServer, &logger, &ledDriver, &wifi, &mqttGateway, &timeServer, &firmwareManager, &dataQueue, &communicationQueueClient);

            samplerQueueClient.begin(communicationQueueClient.getQueueHandle());
            communicationQueueClient.begin(samplerQueueClient.getQueueHandle());
            sampler.setup();
            connection.setup();
            Assert::AreEqual(Led::OFF, Led::get(Led::AUX), L"AUX off");
            Assert::AreEqual(Led::OFF, Led::get(Led::RUNNING), L"RUNNING off");
            communicationEventServer.subscribe(&logger, Topic::Sample);
            sampler.begin();
            sampler.loop();
            while (communicationQueueClient.receive()) { }
            Assert::AreEqual(Led::OFF, Led::get(Led::AUX), L"AUX still off");
            Assert::AreEqual(Led::ON, Led::get(Led::RUNNING), L"RUNNING on");
            Assert::AreEqual("[] Starting\n[] Topic '6': 0\n", Serial.getOutput());
            communicationEventServer.unsubscribe(&logger, Topic::Sample);
            Serial.clearOutput();
            for (int i = 0; i < ResultAggregator::FLATLINE_STREAK - 1; i++) {
                sampler.loop();
                while (communicationQueueClient.receive()) {}
            }
            Assert::AreEqual("[] Flatline: 0\n", Serial.getOutput());


        }
    };
}
