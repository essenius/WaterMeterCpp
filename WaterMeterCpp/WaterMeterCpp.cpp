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

#ifdef ESP32
#include <ESP.h>
#include <PubSubClient.h>
#include <QMC5883LCompass.h>
#else
#include "ArduinoMock.h"
#include "PubSubClientMock.h"
#include "QMC5883LCompassMock.h"
#endif

#include "Communicator.h"
#include "Device.h"
#include "FlowMeter.h"
#include "LedDriver.h"
#include "MagnetoSensorReader.h"
#include "MqttGateway.h"
#include "EventServer.h"
#include "ResultAggregator.h"
#include "TimeServer.h"
#include "Wifi.h"
#include "Connector.h"
#include "FirmwareManager.h"
#include "Log.h"
#include "SampleAggregator.h"
#include "QueueClient.h"
#include "Sampler.h"
#include "secrets.h" // includes config.h
#include "SensorDataQueue.h"

// For being able to set the firmware 
constexpr const char* const BUILD_VERSION = "0.100.3";

// We measure every 10 ms. That is about the fastest that the sensor can do reliably
// Processing one cycle usually takes quite a bit less than that, unless a write happened.
constexpr unsigned long MEASURE_INTERVAL_MICROS = 10UL * 1000UL;

// This is where you would normally use an injector framework,
// but defining globally to not use the heap.

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
SensorDataQueue sensorDataQueue(&connectorEventServer, &connectorPayload);
SensorDataQueuePayload measurementPayload;
SensorDataQueuePayload resultPayload;
SampleAggregator sampleAggregator(&samplerEventServer, &theClock, &sensorDataQueue, &measurementPayload);
ResultAggregator resultAggregator(&samplerEventServer, &theClock, &sensorDataQueue, &resultPayload, MEASURE_INTERVAL_MICROS);

Device device(&communicatorEventServer);
LedDriver ledDriver(&communicatorEventServer);
PayloadBuilder wifiPayloadBuilder;
Log logger(&communicatorEventServer, &wifiPayloadBuilder);

Wifi wifi(&connectorEventServer, &WIFI_CONFIG, &wifiPayloadBuilder);
PubSubClient mqttClient;
MqttGateway mqttGateway(&connectorEventServer, &mqttClient, &MQTT_CONFIG, &sensorDataQueue, BUILD_VERSION);
FirmwareManager firmwareManager(&connectorEventServer, CONFIG_BASE_FIRMWARE_URL, BUILD_VERSION);

QueueClient samplerQueueClient(&samplerEventServer, 20);
QueueClient communicatorSamplerQueueClient(&communicatorEventServer, 20);
QueueClient communicatorConnectorQueueClient(&communicatorEventServer, 20);
QueueClient connectorCommunicatorQueueClient(&connectorEventServer, 100);

// send only, but 1 is the minimum size for a queue
QueueClient connectorSamplerQueueClient(&connectorEventServer, 1);
SensorDataQueuePayload connectorDataQueuePayload;
SensorDataQueuePayload communicatorQueuePayload;
PayloadBuilder serialize2PayloadBuilder(&theClock);
Serializer serializer2(&communicatorEventServer, &serialize2PayloadBuilder);

DataQueue connectorDataQueue(&connectorEventServer, &connectorDataQueuePayload, 1024);
Sampler sampler(&samplerEventServer, &sensorReader, &flowMeter, &sampleAggregator, &resultAggregator, &samplerQueueClient);
Communicator communicator(&communicatorEventServer, &logger, &ledDriver, &device, &connectorDataQueue, &serializer2, &communicatorSamplerQueueClient,
                          &communicatorConnectorQueueClient);

TimeServer timeServer;
Connector connector(&connectorEventServer, &wifi, &mqttGateway, &timeServer, &firmwareManager, &sensorDataQueue, &connectorDataQueue, &serializer, &connectorSamplerQueueClient, &connectorCommunicatorQueueClient);

TaskHandle_t communicatorTaskHandle;
TaskHandle_t connectorTaskHandle;

void setup() {
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
}

void loop() {
    sampler.loop();
}

#ifndef ESP32
int main() {
    setup();
    while (true) loop();
}
#endif
