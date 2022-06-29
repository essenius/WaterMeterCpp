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

#include "TestEventClient.h"
#include "../WaterMeterCpp/SafeCString.h"

namespace WaterMeterCppTest {

    int TestEventClient::getCallCount() const { return _callCount; }

    char* TestEventClient::getPayload() { return _payload; }

    Topic TestEventClient::getTopic() const { return _topic; }

    void TestEventClient::reset() {
        _callCount = 0;
        _topic = Topic::None;
        _payload[0] = 0;
    }

    void TestEventClient::update(const Topic topic, const char* payload) {
        _callCount++;
        _wasLong = false;
        _topic = topic;
        safeStrcpy(_payload, payload);
    }

    void TestEventClient::update(const Topic topic, const long payload) {
        _callCount++;
        _wasLong = true;
        _topic = topic;
        safeSprintf(_payload, "%ld", payload);
    }

    bool TestEventClient::wasLong() const { return _wasLong; }
}
