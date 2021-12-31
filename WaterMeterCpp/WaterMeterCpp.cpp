// Copyright 2021 Rik Essenius
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
#else
#include "ArduinoMock.h"
#endif

#include "Device.h"
#include "FlowMeter.h"
#include "LedDriver.h"
#include "MagnetoSensorReader.h"
#include "MqttGateway.h"
#include "EventServer.h"
#include "ResultWriter.h"
#include "Scheduler.h"
#include "TimeServer.h"
#include "Wifi.h"
#include "FirmwareManager.h"
#include "Log.h"
#include "MeasurementWriter.h"
#include "secrets.h" // for CONFIG_BASE_FIRMWARE_URL


constexpr const char* const BUILD_VERSION = "7";

constexpr unsigned long MEASURE_INTERVAL_MICROS = 10UL * 1000UL;
constexpr unsigned long MEASUREMENTS_PER_MINUTE = 60UL * 1000UL * 1000UL / MEASURE_INTERVAL_MICROS;

EventServer eventServer(LogLevel::Off);
LedDriver ledDriver(&eventServer);

Wifi wifi(&eventServer, CONFIG_SSID, CONFIG_PASSWORD, CONFIG_DEVICE_NAME);
TimeServer timeServer(&eventServer);
Device device(&eventServer);
MagnetoSensorReader sensorReader;
PayloadBuilder measurementPayloadBuilder;
MeasurementWriter measurementWriter(&eventServer, &measurementPayloadBuilder);
FlowMeter flowMeter;
PayloadBuilder resultPayloadBuilder;
ResultWriter resultWriter(&eventServer, &resultPayloadBuilder, MEASURE_INTERVAL_MICROS);
MqttGateway mqttGateway(&eventServer, CONFIG_MQTT_BROKER, CONFIG_MQTT_PORT, CONFIG_MQTT_USER, CONFIG_MQTT_PASSWORD, BUILD_VERSION);
Scheduler scheduler(&eventServer, &measurementWriter, &resultWriter);
FirmwareManager firmwareManager(&eventServer);
Log logger(&eventServer);

unsigned long nextMeasureTime;

bool clearedToGo = false;

void setup() {
    logger.begin();
    ledDriver.begin();
    sensorReader.begin();
    eventServer.publish(Topic::Processing, LONG_TRUE);
    wifi.begin();

#ifdef USE_TLS
    wifi.setCertificates(CONFIG_ROOTCA_CERTIFICATE, CONFIG_DEVICE_CERTIFICATE, CONFIG_DEVICE_PRIVATE_KEY);
#endif

    timeServer.begin();
    firmwareManager.begin(wifi.getClient(), CONFIG_BASE_FIRMWARE_URL, eventServer.request(Topic::MacRaw, "mac-not-found"));

    if (timeServer.timeWasSet()) {
        firmwareManager.tryUpdateFrom(BUILD_VERSION);
        eventServer.publish(Topic::TimeOverrun, LONG_FALSE);
        mqttGateway.begin(wifi.getClient(), wifi.getHostName());
        // the writers need the time, so can't do this earlier. This means connected is true by default
        measurementWriter.begin();
        resultWriter.begin();

        nextMeasureTime = micros();
        clearedToGo = true;
    } else {
        eventServer.publish(Topic::Error, "Could not set time");
    }
}
unsigned int timeout = 0;
unsigned int loopCount = 0;
PayloadBuilder payloadBuilder;

unsigned long maxUsedTime = 0;
unsigned long totalUsedTime = 0;
long peaks = 0;

void loop() {
    if (clearedToGo) {
        unsigned long startLoopTimestamp = micros();
        nextMeasureTime += MEASURE_INTERVAL_MICROS;
        eventServer.publish(Topic::Sample, LONG_TRUE);
        eventServer.publish(Topic::Processing, LONG_TRUE);
        SensorReading measure = sensorReader.read();
        flowMeter.addMeasurement(measure.Y);
        measurementWriter.addMeasurement(measure.Y);
        resultWriter.addMeasurement(measure.Y, &flowMeter);

        scheduler.processOutput();

        eventServer.publish(Topic::ProcessTime, micros() - startLoopTimestamp);
        eventServer.publish(Topic::Processing, LONG_FALSE);

        // not using delayMicroseconds() as that is less accurate. Sometimes up to 300 us too much wait time.
        unsigned long usedTime = micros() - startLoopTimestamp;
        if (usedTime > MEASURE_INTERVAL_MICROS) {
            // It took too long. We might be able to catch up, but intervene if the difference gets too big
            eventServer.publish(Topic::TimeOverrun, usedTime - MEASURE_INTERVAL_MICROS);
            nextMeasureTime += static_cast<int>((micros() - nextMeasureTime) / MEASURE_INTERVAL_MICROS) * MEASURE_INTERVAL_MICROS;
        } else {

            totalUsedTime += usedTime;
            maxUsedTime = usedTime > maxUsedTime ? usedTime : maxUsedTime;

            // fill the remaining time with checking for messages and validating connection
            while (micros() < nextMeasureTime) {
                if (!wifi.isConnected()) {
                    eventServer.publish(Topic::Connected, LONG_FALSE);
                    wifi.begin();
                    if (wifi.isConnected()) {
                        eventServer.publish(Topic::Connected, LONG_TRUE);
                    }
                }
                device.reportHealth();
                mqttGateway.handleQueue();
            }
        }
    }
}

#ifndef ESP32
int main() {
    setup();
    while (true) loop();
}
#endif
