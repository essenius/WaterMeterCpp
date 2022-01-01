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

#ifdef ESP32
#include <Wire.h>
#include <ESP.h>
#else
#include "ArduinoMock.h"
#endif

#include "MagnetoSensorReader.h"


void MagnetoSensorReader::begin() {
    _compass.setCalibration(-1410, 1217, -1495, 1435, -1143, 1680);
    _compass.init();
    // ignore the first measurements, often outliers
    _compass.read();
    delay(10);
    _compass.read();
    delay(10);
    _compass.read();
}

SensorReading MagnetoSensorReader::read() {
    _compass.read();
    _sensorReading.X = _compass.getX();
    _sensorReading.Y = _compass.getY();
    _sensorReading.Z = _compass.getZ();
    return _sensorReading;
}
