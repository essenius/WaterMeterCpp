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

// ReSharper disable CppClangTidyClangDiagnosticExitTimeDestructors

// This project implements a water meter using a QMC5883L compass sensor on an ESP32 board.
// To enable unit testing in Visual Studio, some of the ESP libraries have been mocked in a separate project.

#include <ESP.h>
#include <PubSubClient.h>

#include "Configuration.h"
#include "Communicator.h"
#include "Connector.h"
#include "Device.h"
#include "EventServer.h"
#include "FirmwareManager.h"
#include "FlowMeter.h"
#include "LedDriver.h"
#include "Log.h"
#include "MagnetoSensorHmc.h"
#include "MagnetoSensorQmc.h"
#include "MagnetoSensorNull.h"
#include "MagnetoSensorReader.h"
#include "Meter.h"
#include "MqttGateway.h"
#include "OledDriver.h"
#include "ResultAggregator.h"
#include "SampleAggregator.h"
#include "Sampler.h"
#include "TimeServer.h"
#include "WiFiManager.h"
#include "QueueClient.h"
#include "WiFiClientFactory.h"
#include "Wire.h"

// For being able to set the firmware 
constexpr const char* const BUILD_VERSION = "0.100.8";

// We measure every 10 ms. That is about the fastest that the sensor can do reliably
// Processing one cycle usually takes quite a bit less than that, unless a write happened.
constexpr unsigned long MEASURE_INTERVAL_MICROS = 10UL * 1000UL;

// This is where you would normally use an injector framework,
// We define the objects globally to avoid using (and fragmenting) the heap.
// we do use dependency injection to hide this design decision as much as possible
// (and make testing easier).

MagnetoSensorQmc qmcSensor;
MagnetoSensorHmc hmcSensor;
MagnetoSensorNull nullSensor;
MagnetoSensor* sensor[] = { &qmcSensor, &hmcSensor, &nullSensor };

Preferences preferences;
Configuration configuration(&preferences);
WiFiClientFactory wifiClientFactory(&configuration.tls);
EventServer samplerEventServer;
MagnetoSensorReader sensorReader(&samplerEventServer);
FlowMeter flowMeter(&samplerEventServer);

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
ResultAggregator resultAggregator(&samplerEventServer, &theClock, &sensorDataQueue, &resultPayload, MEASURE_INTERVAL_MICROS);

Device device(&communicatorEventServer);
Meter meter(&communicatorEventServer);
LedDriver ledDriver(&communicatorEventServer);
OledDriver oledDriver(&communicatorEventServer);
PayloadBuilder wifiPayloadBuilder;
Log logger(&communicatorEventServer, &wifiPayloadBuilder);

WiFiManager wifi(&connectorEventServer, &configuration.wifi, &wifiPayloadBuilder);
PubSubClient mqttClient;
MqttGateway mqttGateway(&connectorEventServer, &mqttClient, &wifiClientFactory, &configuration.mqtt, &sensorDataQueue,
                        BUILD_VERSION);
FirmwareManager firmwareManager(&connectorEventServer, &wifiClientFactory, &configuration.firmware, BUILD_VERSION);

QueueClient samplerQueueClient(&samplerEventServer, &logger, 50, 0);
QueueClient communicatorSamplerQueueClient(&communicatorEventServer, &logger, 50, 1);
QueueClient communicatorConnectorQueueClient(&communicatorEventServer, &logger, 50, 2);
// This queue needs more space as it won't be read when offline.
QueueClient connectorCommunicatorQueueClient(&connectorEventServer, &logger, 100, 3);

// Nothing to send from sampler to connector
QueueClient connectorSamplerQueueClient(&connectorEventServer, &logger, 0, 4);
DataQueuePayload connectorDataQueuePayload;
DataQueuePayload communicatorQueuePayload;
PayloadBuilder serialize2PayloadBuilder(&theClock);
Serializer serializer2(&communicatorEventServer, &serialize2PayloadBuilder);

DataQueue connectorDataQueue(&connectorEventServer, &connectorDataQueuePayload, 1, 1024, 128, 256);
Sampler sampler(&samplerEventServer, &sensorReader, &flowMeter, &sampleAggregator, &resultAggregator, &samplerQueueClient);
Communicator communicator(&communicatorEventServer, &logger, &ledDriver, &oledDriver, &meter, &device, &connectorDataQueue, &serializer2,
                          &communicatorSamplerQueueClient, &communicatorConnectorQueueClient);

TimeServer timeServer;
Connector connector(&connectorEventServer, &wifi, &mqttGateway, &timeServer, &firmwareManager, &sensorDataQueue,
                    &connectorDataQueue, &serializer, &connectorSamplerQueueClient, &connectorCommunicatorQueueClient);

TaskHandle_t communicatorTaskHandle;
TaskHandle_t connectorTaskHandle;

void setup() {
    Serial.begin(115200);
    theClock.begin();
    sensorReader.power(HIGH);

    // wait for the sensor to be ready for measurements
    delay(50);
    Wire.begin();

    configuration.begin();
    // queue for the sampler process
    samplerQueueClient.begin(communicatorSamplerQueueClient.getQueueHandle());

    // queues for the communicator process
    // receive only
    communicatorSamplerQueueClient.begin();
    communicatorConnectorQueueClient.begin(connectorCommunicatorQueueClient.getQueueHandle());

    // queues for the connector process
    connectorCommunicatorQueueClient.begin(communicatorConnectorQueueClient.getQueueHandle());
    connectorSamplerQueueClient.begin(samplerQueueClient.getQueueHandle());

    communicator.setup();
    connector.setup(&configuration);

    // ReSharper disable once CppUseStdSize -- we need a C++ 11 compatible way
    sampler.setup(sensor, sizeof sensor / sizeof sensor[0], MEASURE_INTERVAL_MICROS);

    // connect to Wifi, get the time and start the MQTT client. Do this on core 0 (where WiFi runs as well)
    xTaskCreatePinnedToCore(Connector::task, "Connector", 10000, &connector, 1, &connectorTaskHandle, 0);

    // the communicator loop takes care of logging and leds, as well as passing on data to the connector if there is a connection
    xTaskCreatePinnedToCore(Communicator::task, "Communicator", 10000, &communicator, 1, &communicatorTaskHandle, 0);

    // begin can only run when both sampler and connector have finished setup, since it can start publishing right away
    sampler.begin();

    device.begin(xTaskGetCurrentTaskHandle(), communicatorTaskHandle, connectorTaskHandle);
}

void loop() {
    // Start the sampler task which is the only task on core 1 so it should not get interrupted. 
    // As Wifi runs on core 0, we don't want to run this time critical task there.
    // One issue: if you run Serial.printf() from core 0, then tasks running on core 1 might get delayed. 
    // printf() doesn't seem to have that problem, so we use that instead.
    sampler.loop();
}

#ifndef ESP32
int main() {
    setup();
    while (true) loop();
}
#endif
