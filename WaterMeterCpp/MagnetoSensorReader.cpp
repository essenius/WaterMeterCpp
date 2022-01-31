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


MagnetoSensorReader::MagnetoSensorReader(EventServer* eventServer, QMC5883LCompass* compass) : _eventServer(eventServer),
    _compass(compass) {}

void MagnetoSensorReader::begin() const {
    _compass->setCalibration(-1410, 1217, -1495, 1435, -1143, 1680);
    _compass->init();
    // ignore the first measurements, often outliers
    _compass->read();
    delay(10);
    _compass->read();
    delay(10);
    _compass->read();
}

int16_t MagnetoSensorReader::read() {
    _compass->read();
    // Empirically determined that Y gave the best data for this meter
    const auto sample = static_cast<int16_t>(_compass->getY());
    // check whether the sensor still works
    if (sample == _previousSample) {
        _streakCount++;
        // if we have too many of the same results in a row, reset the sensor
        if (_streakCount > FLATLINE_STREAK) {
            _eventServer->publish(Topic::ResetSensor, LONG_TRUE);
            reset();
        }
    }
    else {
        // all good, reset the statistics
        _streakCount = 1;
        _consecutiveStreakCount = 0;
        _previousSample = sample;
    }
    return sample;
}

void MagnetoSensorReader::reset() {
    _consecutiveStreakCount++;
    // If we have done this a number of times in a row, we post an alert
    if (_consecutiveStreakCount >= MAX_STREAKS_TO_ALERT) {
        _eventServer->publish(Topic::Alert, LONG_TRUE);
        _eventServer->publish(Topic::SamplingError, "Sensor does not return values");
        _consecutiveStreakCount = 0;
    }
    // reset the sensor
    _compass->setReset();
    // reset puts the sensor into standby mode, so set it back to continuous
    _compass->setMode(0x01, 0x0C, 0x10, 0X00);
    _streakCount = 1;
}
