// Copyright 2022-2024 Rik Essenius
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
#include <fstream>
#include <sstream>

#include "TestEventClient.h"


namespace WaterMeterCppTest {

    class PulseTestEventClient final : public TestEventClient {
    public:
        explicit PulseTestEventClient(EventServer* eventServer, const char* fileName = nullptr);
        unsigned int anomalies() const { return _excludeCount; }
        unsigned int drifts() const { return _driftCount; }
        unsigned int noFits() const { return _noFitCount; }
        const char* pulseHistory() const { return _buffer; }
        unsigned int pulses(const bool stage) const { return _pulseCount[stage]; }
        void update(Topic topic, long payload) override;
        void writeAttributes();
        void update(Topic topic, SensorSample payload) override;
        void close();

    private:
        char _buffer[4096] = {};
        SensorSample _currentSample = {};
        unsigned int _driftCount = 0;
        unsigned int _excludeCount = 0;
        unsigned int _pulseCount[2] = {};
        unsigned int _noFitCount = 0;
        unsigned int _sampleNumber = -1;
        bool _writeToFile;
        std::ofstream _file;
        std::stringstream _line;
        bool _anomaly = false;
        bool _noFit = false;
        bool _drift = false;
        bool _pulse = false;
    };
}
