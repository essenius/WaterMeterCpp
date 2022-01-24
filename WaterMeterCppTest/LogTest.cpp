// Copyright 2021-2022 Rik Essenius
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

#include <regex>

#include "CppUnitTest.h"
#include "../WaterMeterCpp/Log.h"
#include "../WaterMeterCpp/EventServer.h"
#include "../WaterMeterCpp/TimeServer.h"
#include "../WaterMeterCpp/ArduinoMock.h"
#include "../WaterMeterCpp/ConnectionState.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
    TEST_CLASS(LogTest) {
    public:
        EventServer eventServer;

        TEST_METHOD(logScriptTest) {
            constexpr int SKIP_TIMESTAMP = 28;
            Log log(&eventServer);
            TimeServer timeServer(&eventServer);
            timeServer.begin();
            log.begin();
            Serial.begin(9600);
            publishConnectionState(Topic::Connection, ConnectionState::MqttReady);
            Assert::AreEqual(" MQTT ready\n", Serial.getOutput() + SKIP_TIMESTAMP, L"Connected logs OK");

            Serial.clearOutput();
            eventServer.publish(Topic::Error, "My Message");
            Assert::AreEqual(" Error: My Message\n", Serial.getOutput() + SKIP_TIMESTAMP, L"Error logs OK");

            Serial.clearOutput();
            publishConnectionState(Topic::Connection, ConnectionState::Disconnected);
            Assert::AreEqual(" Disconnected\n", Serial.getOutput() + SKIP_TIMESTAMP, L"Disconnected logs OK");

            Serial.clearOutput();
            Assert::AreEqual("", Serial.getOutput());

            eventServer.publish(Topic::Info, 24);
            Assert::AreEqual(" 24\n", Serial.getOutput() + SKIP_TIMESTAMP, L"Info logs long OK");

            Serial.clearOutput();
            log.update(Topic::BatchSize, 24L);
            Assert::AreEqual(" Topic '1': 24\n", Serial.getOutput() + SKIP_TIMESTAMP, L"Unexpected topic handled OK");

            Serial.clearOutput();
            eventServer.publish(Topic::Alert, 1);
            Assert::AreEqual(" Alert!\n", Serial.getOutput() + SKIP_TIMESTAMP, L"Alert handled OK");

            Serial.clearOutput();
            eventServer.publish(Topic::TimeOverrun, 1234);
            Assert::AreEqual(" Time overrun: 1234\n", Serial.getOutput() + SKIP_TIMESTAMP, L"Time overrun handled OK");

        }
private:
        void publishConnectionState(const Topic topic, ConnectionState connectionState) {
            eventServer.publish(topic, static_cast<long>(connectionState));
        }
    };
}
