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

struct SensorReading {
  int16_t x;
  int16_t y;
  int16_t z;
};

class MagnetoSensorReader : public EventClient {
public:
    explicit MagnetoSensorReader(EventServer* eventServer);
    void begin();
    SensorReading read();
    void update(Topic topic, long payload) override;
    void update(Topic topic, const char* payload) override;

private:
    QMC5883LCompass _compass;
    SensorReading _sensorReading = {};
};

#endif
