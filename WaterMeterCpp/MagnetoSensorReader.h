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

#ifndef HEADER_MAGNETOSENSORREADER
#define HEADER_MAGNETOSENSORREADER

#ifdef ESP32
#include <QMC5883LCompass.h>
#else
#include "QMC5883LCompassMock.h"
#endif

#include "EventServer.h"

class MagnetoSensorReader {
public:
    MagnetoSensorReader(EventServer* eventServer, QMC5883LCompass* compass);
    void begin() const;
    int16_t read();
    void reset();

private:
    static constexpr int FLATLINE_STREAK = 25;
    static constexpr int MAX_STREAKS_TO_ALERT = 10;
    EventServer* _eventServer;
    QMC5883LCompass* _compass;
    int16_t _previousSample = -32768;
    int _streakCount = 0;
    int _consecutiveStreakCount = 0;
};

#endif
