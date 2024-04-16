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

#include "PulseTestEventClient.h"

#include <fstream>
#include <iostream>
#include <SafeCString.h>

namespace WaterMeterCppTest {
    PulseTestEventClient::PulseTestEventClient(EventServer* eventServer, const char* fileName) : TestEventClient(eventServer) {
        _writeToFile = fileName != nullptr;
        if (_writeToFile) {
            _file.open(fileName);
            
            system("cd");
            std::cout << "Writing to " << fileName << std::endl;
            _file << "SampleNo,X,Y,Pulse,Anomaly,NoFit,Drift\n";
        }
        eventServer->subscribe(this, Topic::Anomaly);
        eventServer->subscribe(this, Topic::Drifted);
        eventServer->subscribe(this, Topic::NoFit);
        eventServer->subscribe(this, Topic::Pulse);
        eventServer->subscribe(this, Topic::Sample);
    }

    void PulseTestEventClient::update(const Topic topic, const long payload) {
        TestEventClient::update(topic, payload);
        if (topic == Topic::Pulse) {
            _pulseCount[payload]++;
            char numberBuffer[32];
            SafeCString::sprintf(numberBuffer, "[%d:%d,%d]\n", _sampleNumber, _currentSample.x, _currentSample.y);
            SafeCString::strcat(_buffer, numberBuffer);
            _pulse = true;
        }
        else if (topic == Topic::Anomaly) {
            _excludeCount++;
            _anomaly = true;
        }
        else if (topic == Topic::NoFit) {
            _noFitCount++;
            _noFit = true;
        }
        else if (topic == Topic::Drifted) {
            _driftCount++;
            _drift = true;
        }
    }

    void PulseTestEventClient::writeAttributes() {
        _line << "," << _pulse << "," << _anomaly << "," << _noFit << "," << _drift << "\n";
    }

    void PulseTestEventClient::update(const Topic topic, const SensorSample payload) {
        // always Sample since that has a SensorSample payload
        _sampleNumber++;
        _currentSample = payload;
        if (!_writeToFile) return;
        // write previous sample attributes
        if (_sampleNumber > 0) writeAttributes();
        _line << _sampleNumber << "," << payload.x << "," << payload.y;
        _file << _line.str();
        _line.str("");
        _line.clear();
        _pulse = false;
        _anomaly = false;
        _drift = false;
        _noFit = false;
    }

    void PulseTestEventClient::close() {
        if (_writeToFile) {
            writeAttributes();
            _file << _line.str();
            _file.close();
            std::cout << "Done writing" << "\n";

        }
    }
}