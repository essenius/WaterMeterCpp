// Copyright 2021-2023 Rik Essenius
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

namespace WaterMeterCppTest {
    using WaterMeter::EventClient;
    using WaterMeter::EventServer;
    using WaterMeter::IntCoordinate;
    using WaterMeter::Topic;

    class TestEventClient : public EventClient {
    public:
        explicit TestEventClient(EventServer* eventServer) : EventClient(eventServer) {
            _payload[0] = 0;
        }

        int getCallCount() const;
        char* getPayload();
        Topic getTopic() const;
        void reset();
        void update(Topic topic, IntCoordinate payload) override;
        void update(Topic topic, const char* payload) override;
        void update(Topic topic, long payload) override;
        bool wasLong() const;
    private:
        int _callCount = 0;
        Topic _topic = Topic::None;
        char _payload[512]{};
        bool _wasLong = false;
    };
}
