// Copyright 2021-2023 Rik Essenius
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

MagnetoSensorReader::MagnetoSensorReader(EventServer* eventServer) : 
	EventClient(eventServer),
    _sensorState(eventServer, Topic::SensorState) {}

// returns false if no (real) sensor could be found
bool MagnetoSensorReader::setSensor() {
    // The last one (null sensor) always matches so _sensor can't be nullptr
    for (uint8_t i = 0; i < static_cast<uint8_t>(_sensorListSize); i++) {
        if (_sensorList[i]->isOn()) {
            _sensor = _sensorList[i];
            break;
        }
    }
    if (!_sensor->begin()) return false; 

    constexpr int IgnoreSampleCount = 5;
    // ignore the first measurements, often outliers
    SensorData sample{};
    _sensor->read(sample);
    for (int i = 1; i < IgnoreSampleCount; i++) {
        delay(DelaySensorMillis);
        _sensor->read(sample);
    }
    return true;
}

bool MagnetoSensorReader::begin(MagnetoSensor* sensor[], const size_t listSize) {
    _sensorList = sensor;
    _sensorListSize = listSize;
    pinMode(_powerPort, OUTPUT);
    const auto sensorFound = setPower(HIGH) == SensorState::Ok;
    _eventServer->subscribe(this, Topic::ResetSensor);
    return sensorFound;
}

// Configure the GPIO port used for the sensor setPower if not default (15). 
void MagnetoSensorReader::configurePowerPort(const uint8_t port) {
    _powerPort = port;
    pinMode(_powerPort, OUTPUT);
}

double MagnetoSensorReader::getGain() const {
    return _sensor->getGain();
}

int MagnetoSensorReader::getNoiseRange() const {
    return _sensor->getNoiseRange();
}

void MagnetoSensorReader::hardReset() {
    setPower(LOW);
    setPower(HIGH);

    _flatlineCount = 0;
    _consecutiveStreakCount = 0;
    _eventServer->publish(Topic::SensorWasReset, HardReset);
}

SensorState MagnetoSensorReader::setPower(const uint8_t state) {
    // do nothing if we are already in the right getState
    const auto currentState = digitalRead(_powerPort);
    if (currentState == state && _sensorState == (state == LOW ? SensorState::None : SensorState::Ok)) return _sensorState;
    digitalWrite(_powerPort, state);
    if (state == LOW) {
        _sensor->waitForPowerOff();
        _sensorState = SensorState::None;
        return _sensorState;
    }
    // wait for the sensor to be ready (50 ms for both QMC and HMC)
    delayMicroseconds(StartupMicros);
    auto ok = setSensor();
    if (!ok) {
        _sensorState = SensorState::BeginError;
        return _sensorState;
    }
    auto retryCount = 0;
    while (!((ok = _sensor->handlePowerOn())) && retryCount < 1) {
        retryCount++;
    }
    _sensorState = ok ? SensorState::Ok : SensorState::PowerError;
    return _sensorState;
}

// Run from within timed task, so do not use events and keep short
IntCoordinate MagnetoSensorReader::read() const {
    SensorData sample{};
    if (!_sensor->read(sample)) {
        return IntCoordinate::error();
    }
    return {{ sample.x, sample.y }};
}
 
SensorState MagnetoSensorReader::validate(const IntCoordinate& sample) {

    _sensorState = sample.isSaturated() ? SensorState::Saturated : sample.hasError() ? SensorState::ReadError : SensorState::Ok;

    if (sample == _previousSample || _sensorState != SensorState::Ok ) {
        _flatlineCount++;
        // if we have too many of the same results in a row, signal to reset the sensor
        if (_flatlineCount >= FlatlineStreak) {
            // check whether we need a hard or a soft reset
            _consecutiveStreakCount++;
            _sensorState = _consecutiveStreakCount >= MaxStreaksToAlert || !_sensor->isReal() ? SensorState::NeedsHardReset : SensorState::NeedsSoftReset;
            return _sensorState;
        }
    }
    else {
        // all good, reset the statistics
        _flatlineCount = 0;
        _consecutiveStreakCount = 0;
        _previousSample = sample;
    }
    return _sensorState;
}

void MagnetoSensorReader::softReset() {
    _sensor->softReset();
    _flatlineCount = 0;
    _eventServer->publish(Topic::SensorWasReset, SoftReset);
}

void MagnetoSensorReader::update(const Topic topic, const long payload) {
    if (topic == Topic::ResetSensor && payload != 0) {
        hardReset();
    }
}
