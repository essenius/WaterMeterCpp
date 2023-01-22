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

#ifndef HEADER_MAGNETOSENSORREADER
#define HEADER_MAGNETOSENSORREADER

#include "MagnetoSensor.h"
#include "ChangePublisher.h"

constexpr int SOFT_RESET = 1;
constexpr int HARD_RESET = 2;

class MagnetoSensorReader final : public EventClient {
public:
    explicit MagnetoSensorReader(EventServer* eventServer);
    bool begin(MagnetoSensor* sensor[], size_t listSize);
    void configurePowerPort(uint8_t port);
    double getGain() const;
    int getNoiseRange() const;
    void hardReset();
    void power(uint8_t state) const;
    IntCoordinate read();
    void reset();
    void update(Topic topic, long payload) override;
    static constexpr byte DEFAULT_POWER_PORT = 15;

private:
    static constexpr int FLATLINE_STREAK = 250;
    static constexpr int MAX_STREAKS_TO_ALERT = 10;
    static constexpr int DELAY_SENSOR_MILLIS = 5;

    bool setSensor();

    MagnetoSensor* _sensor = nullptr;
    ChangePublisher<bool> _alert;
    ChangePublisher<bool> _noSensor;
    int _consecutiveStreakCount = 0;
    SensorData _previousSample = {0, 0, 0, 0};
    int _streakCount = 0;
    uint8_t _powerPort = DEFAULT_POWER_PORT;
    MagnetoSensor** _sensorList = nullptr;
    size_t _sensorListSize = 0;
};

#endif
