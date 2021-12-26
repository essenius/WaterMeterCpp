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
//#include "MeasurementWriter.h"
#include "MqttGateway.h"
#include "PubSub.h"
//#include "ResultWriter.h"
//#include "Scheduler.h"
#include "TimeServer.h"
#include "Wifi.h"
#include "FirmwareManager.h"
#include "secrets.h" // for CONFIG_BASE_FIRMWARE_URL


constexpr int BUILD_NUMBER = 4;

constexpr unsigned long MEASURE_INTERVAL_MICROS = 10UL * 1000UL;
constexpr unsigned long MEASUREMENTS_PER_MINUTE = 60UL * 1000UL * 1000UL / MEASURE_INTERVAL_MICROS;

EventServer eventServer(LogLevel::Off);
LedDriver ledDriver(&eventServer);

Wifi wifi;
TimeServer timeServer(&eventServer);
Device device(&eventServer);
MagnetoSensorReader sensorReader;
//PayloadBuilder measurementPayloadBuilder;
//MeasurementWriter measurementWriter(&eventServer, &timeServer, &measurementPayloadBuilder);
FlowMeter flowMeter;
//PayloadBuilder resultPayloadBuilder;
//ResultWriter resultWriter(&eventServer, &timeServer, &resultPayloadBuilder, MEASURE_INTERVAL_MICROS);
MqttGateway mqttGateway(&eventServer);
//Scheduler scheduler(&eventServer, &measurementWriter, &resultWriter);
FirmwareManager firmwareManager;

unsigned long nextMeasureTime;
//unsigned long startLoopTimestamp;

bool clearedToGo = false;

void setup() {
    Serial.begin(115200);
    Serial.println("Starting");
    // start early to switch on led and subscribe to connected event
    ledDriver.begin();
    sensorReader.begin();
    eventServer.publish(Topic::Processing, LONG_TRUE);
    wifi.begin();
    timeServer.begin();
    firmwareManager.begin(wifi.getClient(), CONFIG_BASE_FIRMWARE_URL, wifi.macAddress());

    if (timeServer.timeWasSet()) {
         // force blue led, TODO fix structurally
        eventServer.publish(Topic::TimeOverrun, LONG_TRUE);
        firmwareManager.tryUpdateFrom(BUILD_NUMBER);
        eventServer.publish(Topic::TimeOverrun, LONG_FALSE);
        mqttGateway.begin(wifi.getClient(), wifi.getHostName());
        // the writers need the time, so can't do this earlier. This means connected is true by default
        //measurementWriter.begin();
        //resultWriter.begin();

        eventServer.publish(Topic::Processing, LONG_FALSE);
        nextMeasureTime = micros();
        Serial.printf("Next measure time: %lu\n", nextMeasureTime);
        clearedToGo = true;
    } else {
        eventServer.publish(Topic::Error, "Could not set time");
        Serial.print("Could not set time");
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
        flowMeter.addMeasurement(measure.y);
        //measurementWriter.addMeasurement(measure);
        //resultWriter.addMeasurement(measure.y, &flowMeter);

        if (flowMeter.getPeak()) {
            eventServer.publish(Topic::Peak, LONG_TRUE);
            peaks++;
        }

        //scheduler.processOutput();

        // Do everything we can before the timing moment. Only the reporting of the timing itself is not counted.
        loopCount++;
        if (loopCount >= 10) {
            payloadBuilder.initialize();
            payloadBuilder.writeParam("time", timeServer.getTime());
            payloadBuilder.writeParam("x", measure.x);
            payloadBuilder.writeParam("y", flowMeter.getSmoothValue());
            payloadBuilder.writeParam("z", measure.z);
            payloadBuilder.writeParam("dy", flowMeter.getSmoothDerivative());
            payloadBuilder.writeParam("peaks", peaks);
        }

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

            // If we need to write out, complete the message and do it
            if (loopCount >= 10) {
                payloadBuilder.writeParam("used", totalUsedTime);
                payloadBuilder.writeParam("max", maxUsedTime);
                payloadBuilder.writeGroupEnd();
                eventServer.publish(Topic::Measurement, payloadBuilder.toString());
                loopCount = 0;
                totalUsedTime = 0;
                maxUsedTime = 0;
                peaks = 0;
            }

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
