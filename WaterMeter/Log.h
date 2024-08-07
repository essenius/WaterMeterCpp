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

// Mechanism to log events to the serial port.
// It uses a mutex to ensure that only one process is doing this at a time.

#ifndef HEADER_LOG
#define HEADER_LOG

#include <ESP.h>

#ifndef ESP32
// hack to redirect printf to capture the output
// ReSharper disable once CppInconsistentNaming
#define printf redirectPrintf
#endif

#include "PayloadBuilder.h"

namespace WaterMeter {
    class Log final : public EventClient {
    public:
        using EventClient::update;
        Log(EventServer* eventServer, PayloadBuilder* wifiPayloadBuilder);
        void begin();

        template <typename... Arguments>
        void log(const char* format, const Arguments ... arguments) const {
            // printf doesn't seem to influence other tasks (unlike Serial.printf)
            xSemaphoreTake(_printMutex, portMAX_DELAY);
            printf("[%s] ", getTimestamp());
            printf(format, arguments...);
            printf("\n");
            xSemaphoreGive(_printMutex);
        }

        void update(Topic topic, const char* payload) override;
        void update(Topic topic, long payload) override;

    private:
        PayloadBuilder* _wifiPayloadBuilder;
        long _previousConnectionTopic = -1;
        static SemaphoreHandle_t _printMutex;

        const char* getTimestamp() const;
        void printIndexedPayload(const char* entity, long payload) const;
    };
}
#endif

