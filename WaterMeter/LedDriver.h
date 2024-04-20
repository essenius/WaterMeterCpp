// Copyright 2021-2024 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#ifndef HEADER_LED_DRIVER
#define HEADER_LED_DRIVER

#include "ConnectionState.h"
#include "EventServer.h"
#include "LedFlasher.h"

namespace WaterMeter {
    class LedDriver final : public EventClient {
    public:
        explicit LedDriver(EventServer* eventServer);
        void begin();
        void update(Topic topic, const char* payload) override;
        void connectionUpdate(ConnectionState payload);
        static void timeOverrunUpdate(bool isOn);
        void update(Topic topic, long payload) override;

        // number of samples for led blinking intervals (* 10 ms)
        static constexpr unsigned int ExcludeInterval = 25;
        static constexpr unsigned int IdleInterval = 100;

        // matches interval of connect process
        static constexpr unsigned int ConnectingInterval = 50;

    private:
        LedFlasher _connectingFlasher;
        LedFlasher _sampleFlasher;
    };
}
#endif
