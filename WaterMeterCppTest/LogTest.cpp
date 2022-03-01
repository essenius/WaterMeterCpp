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
#include "../WaterMeterCpp/Clock.h"
#include "../WaterMeterCpp/ArduinoMock.h"
#include "../WaterMeterCpp/ConnectionState.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
    TEST_CLASS(LogTest) {
    public:
        EventServer eventServer;

        TEST_METHOD(logScriptTest) {
            setLogLevel(verbose);
            PayloadBuilder payloadBuilder;
            Log log(&eventServer, &payloadBuilder);
            Clock theClock(&eventServer);
            theClock.begin();
            log.begin();
            Serial.begin(9600);
            Serial.setInput(""); // just so it's used and doesn't break

            log.testLogMacro();
            // the pattern we expect here is [2022-02-22T01:02:03.456789][Q] {hello}\r\n
            Assert::IsTrue(
                std::regex_match(
                    Serial.getOutput(),
                    std::regex(R"(\[\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{6}\]\[Q\]\s\{hello\}\r\n)")),
                L"Log pattern matches");
            Serial.clearOutput();

            publishConnectionState(Topic::Connection, ConnectionState::MqttReady);
            Assert::AreEqual("[I] MQTT ready\n", Serial.getOutput(), L"Connected logs OK");

            Serial.clearOutput();
            eventServer.publish(Topic::MessageFormatted, "My Message");
            Assert::AreEqual("[I] My Message\n", Serial.getOutput(), L"Message logs OK");

            Serial.clearOutput();
            publishConnectionState(Topic::Connection, ConnectionState::Disconnected);
            Assert::AreEqual("[I] Disconnected\n", Serial.getOutput(), L"Disconnected logs OK");

            Serial.clearOutput();
            Assert::AreEqual("", Serial.getOutput());

            eventServer.publish(Topic::MessageFormatted, 24);
            Assert::AreEqual("[I] 24\n", Serial.getOutput(), L"MessageFormatted accepts long OK");

            Serial.clearOutput();
            log.update(Topic::BatchSize, 24L);
            Assert::AreEqual("[I] Topic '1': 24\n", Serial.getOutput(), L"Unexpected topic handled OK");

            Serial.clearOutput();
            eventServer.publish(Topic::Alert, 1);
            Assert::AreEqual("[W] Alert: 1\n", Serial.getOutput(), L"Alert handled OK");

            Serial.clearOutput();
            eventServer.publish(Topic::TimeOverrun, 1234);
            Assert::AreEqual("[W] Time overrun: 1234\n", Serial.getOutput(), L"Time overrun handled OK");

            Serial.clearOutput();
            eventServer.publish(Topic::ResultWritten, LONG_TRUE);
            Assert::AreEqual("[D] Result Written: 1\n", Serial.getOutput(), L"Result Written handled OK");

            Serial.clearOutput();
            eventServer.publish(Topic::Blocked, LONG_TRUE);
            Assert::AreEqual("[E] Blocked: 1\n", Serial.getOutput(), L"Blocked handled OK");

            Serial.clearOutput();
            eventServer.publish(Topic::SensorWasReset, LONG_TRUE);
            Assert::AreEqual("[W] Sensor was reset\n", Serial.getOutput(), L"Sensor reset handled OK");

            Serial.clearOutput();
            eventServer.publish(Topic::FreeQueueSpaces, 0x03000010);
            Assert::AreEqual("[I] Free Spaces Queue #3: 16\n", Serial.getOutput(), L"Sensor reset handled OK");
            setLogLevel(info);

        }
private:
        void publishConnectionState(const Topic topic, ConnectionState connectionState) {
            eventServer.publish(topic, static_cast<long>(connectionState));
        }
    };
}
