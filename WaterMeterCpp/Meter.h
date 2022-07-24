// Copyright 2022 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#ifndef HEADER_METER
#define HEADER_METER

#include "EventClient.h"

class Meter final : public EventClient {
public:
    explicit Meter(EventServer* eventServer);
    void begin();
    const char* getVolume();
    void newPulse();
    void publishValues();
    bool setVolume(const char* meterValue);
    void update(Topic topic, const char* payload) override;
    void update(Topic topic, long payload) override;

private:
    // 31,308 pulses per cubic meter (i.e. 1000L) - determined empirically (was 33173)
    static constexpr double PULSES_PER_UNIT = 31308;
    static constexpr double PULSE_DELTA = 1.0 / PULSES_PER_UNIT;
    double _volume = 0.0;
    unsigned long _pulses = 0;
    static constexpr int BUFFERSIZE = 20;
    char _buffer[BUFFERSIZE] = "";
};
#endif
