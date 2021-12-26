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

#ifdef ESP32
#include <Wire.h>
#include <QMC5883LCompass.h>
#include <ESP.h>
#else
#include "QMC5883LCompassMock.h"
#include "ArduinoMock.h"
#endif

#include "MagnetoSensorReader.h"

QMC5883LCompass compass;

void MagnetoSensorReader::begin() {
    //pinMode(SDA, INPUT_PULLUP);
    //pinMode(SCL, INPUT_PULLUP);
    compass.setCalibration(-1410, 1217, -1495, 1435, -1143, 1680);
    compass.init();
    // ignore the first measurements, often outliers
    compass.read();
    delay(10);
    compass.read();
    delay(10);
    compass.read();
}

SensorReading MagnetoSensorReader::read() {
    compass.read();
    sensorReading.x = compass.getX();
    sensorReading.y = compass.getY();
    sensorReading.z = compass.getZ();
    return sensorReading;
}
