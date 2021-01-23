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

#ifdef ESP8266
#include <ESP.h>
#include <ESP8266WiFi.h>
#elif ARDUINO
#include <Arduino.h>
#else
#include "Arduino.h"
#endif

#include "FlowMeter.h"
#include "LedDriver.h"
#include "MagnetoSensorReader.h"
#include "MeasurementWriter.h"
#include "ResultWriter.h"
#include "SerialDriver.h"
#include "SerialScheduler.h"
#include "Storage.h"

const int MEASURE_INTERVAL_MICROS = 10000;

#ifdef ESP8266
int freeMemory() {
  return ESP.getFreeHeap();
}
#elif ARDUINO
int freeMemory() {
    char top;
    return &top - __malloc_heap_start;
}
#else
int freeMemory() {
    return 4000;
}
#endif

// do LedDriver first so it switches on the led, showing we are setting up.
LedDriver ledDriver;
MagnetoSensorReader sensorReader;
SerialDriver serialDriver;
Storage storage;
MeasurementWriter measurementWriter(&storage);
BatchWriter infoWriter(1, 'I');
FlowMeter flowMeter;
ResultWriter resultWriter(&storage, MEASURE_INTERVAL_MICROS);
SerialScheduler scheduler(&serialDriver, &measurementWriter, &resultWriter, &infoWriter, &freeMemory);

unsigned long usedTime = 0;
unsigned long nextMeasureTime;
unsigned long startLoopTimestamp;

void setup() {
#ifdef ESP8266
    WiFi.mode( WIFI_OFF );
#endif
    ledDriver.begin();
    serialDriver.begin();
    sensorReader.begin();
    nextMeasureTime = micros();
    ledDriver.toggleBuiltin();
}

unsigned int timeout = 0;

void loop() {
    unsigned long startLoopTimestamp = micros();
    nextMeasureTime += MEASURE_INTERVAL_MICROS;

    int measure = sensorReader.read();
    flowMeter.addMeasurement(measure);
    ledDriver.signalMeasurement(flowMeter.isExcluded(), flowMeter.hasFlow());
    measurementWriter.addMeasurement(measure, usedTime);
    resultWriter.addMeasurement(measure, usedTime, &flowMeter);
    bool hasInput = scheduler.processInput();
    bool hasOutput = scheduler.processOutput();
    ledDriver.signalConnected(scheduler.isConnected());
    ledDriver.signalError(scheduler.hasDelayedFlush());
    ledDriver.signalFlush(scheduler.wasResultWritten());
    if (freeMemory() < 2048) {
        if (timeout == 0) {
            infoWriter.newMessage();
            infoWriter.writeParam("Warning: low on memory");
            infoWriter.writeParam(freeMemory());
            timeout = 1000;
        } else if (timeout > 0) {
            timeout--;
        }
    }

    usedTime = micros() - startLoopTimestamp;
    // not using delayMicroseconds() as that is less accurate. Sometimes up to 300 us too much wait time.
    if (micros() > nextMeasureTime) {
        // It took too long. We might be able to catch up, but intervene if the difference gets too big
        nextMeasureTime += int((nextMeasureTime - micros()) / MEASURE_INTERVAL_MICROS) * MEASURE_INTERVAL_MICROS;
    }
    else {
        while (micros() < nextMeasureTime) {};
    }
}

#ifndef ARDUINO
int main() {
    setup();
    while (true) loop();
}
#endif
