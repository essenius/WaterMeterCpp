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
#include "MagnetoSensorQmc.h"

MagnetoSensorReader::MagnetoSensorReader(EventServer* eventServer) :
    EventClient(eventServer),
    _alert(eventServer, Topic::Alert),
    _noSensor(eventServer, Topic::NoSensorFound) {

}

bool MagnetoSensorReader::setSensor() {
    // The last one (null sensor) always matches so _sensor can't be nullptr
    for (uint8_t i = 0; i < static_cast<uint8_t>(_sensorListSize); i++) {
        if (_sensorList[i]->isOn()) {
            _sensor = _sensorList[i];
            break;
        }
    }
    if (!_sensor->begin()) {
        _noSensor = true;
        _alert = true;
        return false;
    }

    constexpr int IGNORE_SAMPLE_COUNT = 5;
    // ignore the first measurements, often outliers
    SensorData sample{};
    _sensor->read(&sample);
    for (int i = 1; i < IGNORE_SAMPLE_COUNT; i++) {
        delay(DELAY_SENSOR_MILLIS);
        _sensor->read(&sample);
    }
    return true;
}

bool MagnetoSensorReader::begin(MagnetoSensor* sensor[], const size_t listSize) {
    _sensorList = sensor;
    _sensorListSize = listSize;
    pinMode(_powerPort, OUTPUT);
    const auto sensorFound = setSensor();
    _eventServer->subscribe(this, Topic::ResetSensor);
    return sensorFound;
}

// Configure the GPIO port used for the sensor power if not default (15). 
void MagnetoSensorReader::configurePowerPort(const uint8_t port) {
    _powerPort = port;
    pinMode(_powerPort, OUTPUT);
}

float MagnetoSensorReader::getGain() const {
    return _sensor->getGain();
}

int MagnetoSensorReader::getNoiseRange() const {
    return _sensor->getNoiseRange();
}

// power cycle the sensor
void MagnetoSensorReader::hardReset() {
    power(LOW);
    if (!_sensor->isReal()) {
        delay(DELAY_SENSOR_MILLIS);
    }
    else {
        while (_sensor->isOn()) {}
    }
    power(HIGH);
    delay(DELAY_SENSOR_MILLIS);
    _noSensor = false;
    setSensor();
    
    _streakCount = 0;
    _consecutiveStreakCount = 0;
    _eventServer->publish(Topic::SensorWasReset, HARD_RESET);
}

void MagnetoSensorReader::power(const uint8_t state) const {
    digitalWrite(_powerPort, state);
}

int16_t MagnetoSensorReader::read() {
    SensorData sample{};
    if (!_sensor->read(&sample)) {
        _alert = true;
        _noSensor = true;
    }
    // Empirically determined that Y gave the best data for this meter
    // check whether the sensor still works
    if (sample.y == _previousSample) {
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
        _previousSample = sample.y;
        _alert = false;
    }
    return sample.y;
}

void MagnetoSensorReader::reset() {
    _consecutiveStreakCount++;
    // If we have done this a number of times in a row, we do a hard reset and post an alert
    if (_consecutiveStreakCount >= MAX_STREAKS_TO_ALERT || !_sensor->isReal()) {
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

void MagnetoSensorReader::update(const Topic topic, long payload) {
    if (topic == Topic::ResetSensor) {
        hardReset();
    }
}
