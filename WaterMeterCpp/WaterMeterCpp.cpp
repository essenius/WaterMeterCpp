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
#include "Connection.h"
#include "FirmwareManager.h"
#include "Log.h"
#include "MeasurementWriter.h"
#include "secrets.h" // for all CONFIG constants


// TODO: this main part is way too big. Fix that.

// For being able to set the firmware 
constexpr const char* const BUILD_VERSION = "0.99.1";

// We measure every 10 ms. That is about the fastest that the sensor can do reliably
// Processing one cycle takes about 8ms max, so that is also within the limit.
constexpr unsigned long MEASURE_INTERVAL_MICROS = 10UL * 1000UL;
constexpr unsigned long MEASUREMENTS_PER_MINUTE = 60UL * 1000UL * 1000UL / MEASURE_INTERVAL_MICROS;
constexpr signed long MIN_MICROS_FOR_CHECKS = MEASURE_INTERVAL_MICROS / 5L;

EventServer eventServer(LogLevel::Off);
LedDriver ledDriver(&eventServer);

Wifi wifi(&eventServer, CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD, CONFIG_DEVICE_NAME, CONFIG_WIFI_BSSID);
TimeServer timeServer(&eventServer);
Device device(&eventServer);
MagnetoSensorReader sensorReader(&eventServer);
PayloadBuilder measurementPayloadBuilder;
MeasurementWriter measurementWriter(&eventServer, &measurementPayloadBuilder);
FlowMeter flowMeter;
PayloadBuilder resultPayloadBuilder;
ResultWriter resultWriter(&eventServer, &resultPayloadBuilder, MEASURE_INTERVAL_MICROS);
MqttGateway mqttGateway(&eventServer, CONFIG_MQTT_BROKER, CONFIG_MQTT_PORT, CONFIG_MQTT_USER, CONFIG_MQTT_PASSWORD, BUILD_VERSION);
Connection connection(&eventServer, &wifi, &mqttGateway);
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
#ifdef CONFIG_USE_STATIC_IP
    wifi.configure(CONFIG_LOCAL_IP, CONFIG_GATEWAY, CONFIG_SUBNETMASK, CONFIG_DNS_1, CONFIG_DNS_2);
#endif

    // run the state machine until connected to wifi
    while (connection.connectWifi() != ConnectionState::WifiConnected) {}

#ifdef CONFIG_USE_TLS
    wifi.setCertificates(CONFIG_ROOTCA_CERTIFICATE, CONFIG_DEVICE_CERTIFICATE, CONFIG_DEVICE_PRIVATE_KEY);
#endif

    timeServer.begin();
    firmwareManager.begin(wifi.getClient(), CONFIG_BASE_FIRMWARE_URL, eventServer.request(Topic::MacRaw, "mac-not-found"));

    if (timeServer.timeWasSet()) {
        firmwareManager.tryUpdateFrom(BUILD_VERSION);
        eventServer.publish(Topic::TimeOverrun, LONG_FALSE);
        //mqttGateway.begin(wifi.getClient(), wifi.getHostName());

        while (connection.connectMqtt() != ConnectionState::MqttReady) {}

        measurementWriter.begin();
        resultWriter.begin();
        nextMeasureTime = micros();
        clearedToGo = true;
    } else {
        eventServer.publish(Topic::Error, "Could not set time");
    }
}

void loop() {
    if (clearedToGo) {
        const unsigned long startLoopTimestamp = micros();
        nextMeasureTime += MEASURE_INTERVAL_MICROS;
        eventServer.publish(Topic::Sample, LONG_TRUE);
        eventServer.publish(Topic::Processing, LONG_TRUE);
        const SensorReading measure = sensorReader.read();
        flowMeter.addMeasurement(measure.Y);
        measurementWriter.addMeasurement(measure.Y);
        resultWriter.addMeasurement(measure.Y, &flowMeter);

        scheduler.processOutput();

        eventServer.publish(Topic::ProcessTime, micros() - startLoopTimestamp);
        eventServer.publish(Topic::Processing, LONG_FALSE);

        // not using delayMicroseconds() as that is less accurate. Sometimes up to 300 us too much wait time.
        // make sure to use durations to compare, not timestamps -- to avoid overflow issues

        const unsigned long usedTime = micros() - startLoopTimestamp;
        if (usedTime > MEASURE_INTERVAL_MICROS) {
            // It took too long. If we're still within one interval, we might be able to catch up
            // Intervene if it gets more than that
            eventServer.publish(Topic::TimeOverrun, usedTime - MEASURE_INTERVAL_MICROS);
            const unsigned long extraTimeNeeded = micros() - nextMeasureTime;
            nextMeasureTime += (extraTimeNeeded / MEASURE_INTERVAL_MICROS) * MEASURE_INTERVAL_MICROS;
        } else {

            // fill remaining time with validating the connection, checking health and handling incoming messages,
            // but only if we have a minimum amount of time left. We don't want overruns because of this.
            // Interval overruns could happen if we get a disconnect, but not much we can do about that (mqtt connects synchronously).

            // We use a trick to avoid micros() overrun issues. Create a duration using unsigned long comparison, and then cast to a
            // signed value. We will not pass LONG_MAX in the duration; ESP32 has 64 bit longs and micros() only uses 32 of those.
            
            auto remainingTime = static_cast<long>(nextMeasureTime - micros());
            if (remainingTime >= MIN_MICROS_FOR_CHECKS) {
                if (connection.connectMqtt() == ConnectionState::MqttReady) {
                    device.reportHealth();
                    mqttGateway.handleQueue();
                }
                do {
                    remainingTime = static_cast<long>(nextMeasureTime - micros());
                } while (remainingTime > 0L);
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
