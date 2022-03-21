// Copyright 2021 Rik Essenius
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

#include "../WaterMeterCpp/EventServer.h"

class TestEventClient : public EventClient {
public:
    explicit TestEventClient(EventServer* eventServer) : EventClient(eventServer) {
        _payload[0] = 0;
    }

    void reset();
    void update(Topic topic, const char* payload) override;
    void update(Topic topic, long payload) override;
    Topic getTopic() const;
    char* getPayload();
    int getCallCount() const;
private:
    int _callCount = 0;
    Topic _topic = Topic::None;
    char _payload[512]{};
};
