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
#else
#include "ArduinoMock.h"
#include "PubSubClientMock.h"
#endif

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
#include "secrets.h" // for all CONFIG constants

// For being able to set the firmware 
constexpr const char* const BUILD_VERSION = "0.100.2";

// We measure every 10 ms. That is about the fastest that the sensor can do reliably
// Processing one cycle takes about 8ms max, so that is also within the limit.
constexpr unsigned long MEASURE_INTERVAL_MICROS = 10UL * 1000UL;

EventServer samplerEventServer;
EventServer communicatorEventServer;
LedDriver ledDriver(&communicatorEventServer);
Wifi wifi(&communicatorEventServer, &WIFI_CONFIG);
TimeServer timeServer(&communicatorEventServer);

Device device(&samplerEventServer);
MagnetoSensorReader sensorReader(&samplerEventServer);
FlowMeter flowMeter(&samplerEventServer);
RingbufferPayload measurementPayload;
PayloadBuilder payloadBuilder;
Serializer serializer(&payloadBuilder);
DataQueue dataQueue(&communicatorEventServer, &serializer);
RingbufferPayload resultPayload;
PubSubClient mqttClient;
MqttGateway mqttGateway(&communicatorEventServer, &mqttClient, &MQTT_CONFIG, &dataQueue, BUILD_VERSION);
FirmwareManager firmwareManager(&communicatorEventServer, CONFIG_BASE_FIRMWARE_URL, BUILD_VERSION);
Log logger(&communicatorEventServer);
QueueClient samplerQueueClient(&samplerEventServer);
QueueClient communicatorQueueClient(&communicatorEventServer);

SampleAggregator sampleAggregator(&samplerEventServer, &dataQueue, &measurementPayload);
ResultAggregator resultAggregator(&samplerEventServer, &dataQueue, &resultPayload, MEASURE_INTERVAL_MICROS);

Sampler sampler(&samplerEventServer, &sensorReader, &flowMeter, &sampleAggregator, &resultAggregator, &device, &samplerQueueClient);
Connector connector(&communicatorEventServer, &logger, &ledDriver, &wifi, &mqttGateway, &timeServer, &firmwareManager, &dataQueue, &communicatorQueueClient);

TaskHandle_t connectorTaskHandle;
TaskHandle_t communicatorTaskHandle;

constexpr bool NEEDS_CONNECTION = true;

void communicatorLoop(void* parameter) {
  for (;;){
      while (communicatorQueueClient.receive(NEEDS_CONNECTION)) {}
      delay(10);
  }
}

void setup() {
    Serial.begin(115200);
    samplerQueueClient.begin(communicatorQueueClient.getQueueHandle());
    communicatorQueueClient.begin(samplerQueueClient.getQueueHandle());

    sampler.setup();
    connector.setup();

    // connect to Wifi, get the time and start the MQTT client. Do this on core 0 (setup and loop run on core 1)
    xTaskCreatePinnedToCore(Connector::task, "Connector", 10000, &connector, 1, &connectorTaskHandle, 0);
    xTaskCreatePinnedToCore(communicatorLoop, "Communicator", 10000, nullptr, 1, &communicatorTaskHandle, 0);

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
