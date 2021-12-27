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
#ifndef HEADER_LEDDRIVER
#define HEADER_LEDDRIVER

#include "EventServer.h"

class LedDriver : public EventClient {
public:
    explicit LedDriver(EventServer* eventServer);
    void begin();
    void update(Topic topic, const char* payload) override;
    void update(Topic topic, long payload) override;

    static constexpr unsigned char AUX_LED = 19;
    static constexpr unsigned char BLUE_LED = 16;
    static constexpr unsigned char GREEN_LED = 17;
    static constexpr unsigned char RED_LED = 18;

    // number of samples for led blinking intervals
    static constexpr unsigned int EXCLUDE_INTERVAL = 25;
    static constexpr unsigned int FLOW_INTERVAL = 50;
    static constexpr unsigned int WAIT_INTERVAL = 100;

private:
    unsigned int _interval = 0;
    unsigned int _ledCounter = 0;
    unsigned int _newInterval = 0;
    uint8_t convertToState(const char* state);
    void signalMeasurement();
};

#endif
