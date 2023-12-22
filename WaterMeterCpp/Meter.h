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

// Translate the pulses to a meter value. 
#ifndef HEADER_METER
#define HEADER_METER

#include "EventClient.h"

namespace WaterMeter {
    class Meter final : public EventClient {
    public:
        explicit Meter(EventServer* eventServer);
        void begin();
        const char* getVolume();
        void newPulse();
        void publishValues();
        bool setVolume(const char* meterValue, double addition = 0.0);
        void update(Topic topic, const char* payload) override;
        void update(Topic topic, long payload) override;

    private:
        // 1 pulse per cycle, this is cycles per 1000 L
        // This needs to be calibrated.
        static constexpr double PulsesPerUnit = 16500;
        static constexpr double PulseDelta = 1.0 / PulsesPerUnit;
        double _volume = 0.0;
        unsigned long _pulses = 0;
        static constexpr int Buffersize = 20;
        char _buffer[Buffersize] = "";
    };
}
#endif
