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

#ifndef HEADER_MAGNETOSENSORREADER
#define HEADER_MAGNETOSENSORREADER

#include "MagnetoSensor.h"
#include "EventServer.h"
#include "ChangePublisher.h"

constexpr int SOFT_RESET = 1;
constexpr int HARD_RESET = 2;

class MagnetoSensorReader final : public EventClient {
public:
    MagnetoSensorReader(EventServer* eventServer, MagnetoSensor** sensor);
    void begin();
    void hardReset();
    bool hasSensor() const;
    int16_t read();
    void reset();
    void update(Topic topic, long payload) override;

private:
    static constexpr int FLATLINE_STREAK = 100;
    static constexpr int MAX_STREAKS_TO_ALERT = 10;
    MagnetoSensor** _sensor;
    ChangePublisher<bool> _alert;
    int _consecutiveStreakCount = 0;
    int16_t _previousSample = -32768;
    int _streakCount = 0;
};

#endif
