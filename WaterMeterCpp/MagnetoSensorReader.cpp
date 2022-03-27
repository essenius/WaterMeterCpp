// Copyright 2021-2022 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#include <Wire.h>
#include <ESP.h>

#include "MagnetoSensorReader.h"

MagnetoSensorReader::MagnetoSensorReader(EventServer* eventServer, MagnetoSensor* sensor) :
    EventClient(eventServer), _sensor(sensor), _alert(eventServer, Topic::Alert) {}

void MagnetoSensorReader::begin() {
    //_sensor->setCalibration(-1410, 1217, -1495, 1435, -1143, 1680);
    _sensor->begin();
    // ignore the first measurements, often outliers
    SensorData sample{};
    _sensor->read(&sample);
    delay(10);
    _sensor->read(&sample);
    delay(10);
    _sensor->read(&sample);
    _eventServer->subscribe(this, Topic::ResetSensor);
}

int16_t MagnetoSensorReader::read() {
    SensorData sample{};
    _sensor->read(&sample);
    // Empirically determined that Y gave the best data for this meter
    // check whether the sensor still works
    if (sample.y == _previousSample) {
        _streakCount++;
        // if we have too many of the same results in a row, softReset the sensor
        if (_streakCount >= FLATLINE_STREAK) {
            reset();
        }
    }
    else {
        // all good, softReset the statistics
        _streakCount = 0;
        _consecutiveStreakCount = 0;
        _previousSample = sample.y;
        _alert = false;
    }
    return sample.y;
}

void MagnetoSensorReader::reset() {
    _consecutiveStreakCount++;
    // If we have done this a number of times in a row, we do a hard softReset and post an alert
    if (_consecutiveStreakCount >= MAX_STREAKS_TO_ALERT) {
        _alert = true;
        hardReset();
        return;
    }
    if (_consecutiveStreakCount == MAX_STREAKS_TO_ALERT / 2) {
        // stop the alert halfway through
        _alert = false;
    }
    _sensor->softReset();
    _streakCount = 0;
    _eventServer->publish(Topic::SensorWasReset, SOFT_RESET);
}

void MagnetoSensorReader::hardReset() {
    _sensor->hardReset();
    _streakCount = 0;
    _consecutiveStreakCount = 0;
    _eventServer->publish(Topic::SensorWasReset, HARD_RESET);
}

void MagnetoSensorReader::update(const Topic topic, long payload) {
    if (topic == Topic::ResetSensor) {
        hardReset();
    }
}
