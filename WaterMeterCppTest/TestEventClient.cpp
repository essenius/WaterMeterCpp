// Copyright 2021 Rik Essenius
// 
//   Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//   except in compliance with the License. You may obtain a copy of the License at
// 
//       http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software distributed under the License
//    is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and limitations under the License.

#include "pch.h"
#include "TestEventClient.h"


void TestEventClient::reset() {
    _callCount = 0;
    _topic = Topic::None;
    _payload[0] = 0;
}

void TestEventClient::update(Topic topic, const char* payload) {
    _callCount++;
    _topic = topic;
    strcpy(_payload, payload);
}

void TestEventClient::update(Topic topic, long payload) {
    _callCount++;
    _topic = topic;
    sprintf(_payload, "%ld", payload);
}

Topic TestEventClient::getTopic() { return _topic; }

char* TestEventClient::getPayload() { return _payload; }

int TestEventClient::getCallCount() { return _callCount; }

