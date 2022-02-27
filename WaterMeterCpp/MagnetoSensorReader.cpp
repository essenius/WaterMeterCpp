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


MagnetoSensorReader::MagnetoSensorReader(EventServer* eventServer, QMC5883LCompass* compass) : 
    EventClient(eventServer), _compass(compass), _alert(eventServer, Topic::Alert) {}

void MagnetoSensorReader::begin() {
    _compass->setCalibration(-1410, 1217, -1495, 1435, -1143, 1680);
    _compass->init();
    // ignore the first measurements, often outliers
    _compass->read();
    delay(10);
    _compass->read();
    delay(10);
    _compass->read();
    _eventServer->subscribe(this, Topic::ResetSensor);
}

int16_t MagnetoSensorReader::read() {
    _compass->read();
    // Empirically determined that Y gave the best data for this meter
    const auto sample = static_cast<int16_t>(_compass->getY());
    // check whether the sensor still works
    if (sample == _previousSample) {
        _streakCount++;
        // if we have too many of the same results in a row, reset the sensor
        if (_streakCount >= FLATLINE_STREAK) {
            reset();
        }
    }
    else {
        // all good, reset the statistics
        _streakCount = 0;
        _consecutiveStreakCount = 0;
        _previousSample = sample;
        _alert = false;

    }
    return sample;
}

void MagnetoSensorReader::reset() {
    _consecutiveStreakCount++;
    // If we have done this a number of times in a row, we post an alert
    if (_consecutiveStreakCount >= MAX_STREAKS_TO_ALERT) {
        _alert = true;
        _consecutiveStreakCount = 0;
    } else if (_consecutiveStreakCount == MAX_STREAKS_TO_ALERT / 2){
      // stop the alert halfway through
        _alert = false;
    }
    // reset the sensor
    _compass->setReset();
    // reset puts the sensor into standby mode, so set it back to continuous
    // 0x01 = continuous (0x00 = standby)
    // 0x0c = data rate 200Hz (0x00=10Hz, 0x04=50Hz,0x08=100Hz)
    // 0x10 = range 8G (0x00 = 2G)
    // 0x00 = oversampling ratio 512 (0x40=256, 0x80=128, 0xc0=64)
    _compass->setMode(0x01, 0x0C, 0x10, 0x00);
    _streakCount = 0;
    _eventServer->publish(Topic::SensorWasReset, LONG_TRUE);
}

void MagnetoSensorReader::update(const Topic topic, long payload) {
    if (topic == Topic::ResetSensor) {
        reset();
    }
}
