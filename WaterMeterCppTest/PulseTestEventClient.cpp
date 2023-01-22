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

#include "PulseTestEventClient.h"

WaterMeterCppTest::PulseTestEventClient::PulseTestEventClient(EventServer* eventServer): TestEventClient(eventServer) {
    eventServer->subscribe(this, Topic::Pulse);
    eventServer->subscribe(this, Topic::Sample);
    eventServer->subscribe(this, Topic::Exclude);
    eventServer->subscribe(this, Topic::NoFit);
}

void WaterMeterCppTest::PulseTestEventClient::update(const Topic topic, const long payload) {
    TestEventClient::update(topic, payload);
    // always Pulse since that has a long payload
    if (topic == Topic::Pulse) {
        _pulseCount[payload]++;
        char numberBuffer[32];
        safeSprintf(numberBuffer, "[%d:%d,%d]\n", _sampleNumber, _currentCoordinate.x, _currentCoordinate.y);
        safeStrcat(_buffer, numberBuffer);
    } else if (topic == Topic::Exclude) {
        _excludeCount++;
    } else if (topic == Topic::NoFit) {
        _noFitCount++;
    }
}

void WaterMeterCppTest::PulseTestEventClient::update(const Topic topic, const IntCoordinate payload) {
    // always Sample since that has an IntCoordinate payload
    _sampleNumber++;
    _currentCoordinate = payload;
}
