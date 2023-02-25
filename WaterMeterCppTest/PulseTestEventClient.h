// Copyright 2022-2023 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#pragma once
#include "TestEventClient.h"


namespace WaterMeterCppTest {
    class PulseTestEventClient final : public TestEventClient {
    public:
        explicit PulseTestEventClient(EventServer* eventServer);
        void update(Topic topic, long payload) override;
        void update(Topic topic, IntCoordinate payload) override;
        const char* pulseHistory() const { return _buffer; }
        unsigned int pulses(const bool stage) const { return _pulseCount[stage]; }
        unsigned int anomalies() const { return _excludeCount; }
        unsigned int noFits() const { return _noFitCount; }
    private:
        char _buffer[4096] = {};
        unsigned int _sampleNumber = -1;
        IntCoordinate _currentCoordinate = {};
        unsigned int _pulseCount[2] = { 0 };
        unsigned int _excludeCount = 0;
        unsigned int _noFitCount = 0;
    };
}
