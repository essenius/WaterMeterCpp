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

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
    TEST_CLASS(LogTest) {
    public:
        EventServer eventServer;

        TEST_METHOD(logScriptTest) {
            Log log(&eventServer);
            TimeServer timeServer(&eventServer);
            timeServer.begin();
            log.begin();
            Serial.begin(9600);
            eventServer.publish(Topic::Connected, 0);

            Assert::IsTrue(std::regex_match(
                               Serial.getOutput(),
                               std::regex(R"(\[\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{6}\]\sConnected\n)")),
                           L"Connected logs OK");

            Serial.clearOutput();
            eventServer.publish(Topic::Error, "My Message");
            Assert::IsTrue(std::regex_match(
                               Serial.getOutput(),
                               std::regex(R"(\[.*\]\sError:\sMy\sMessage\n)")),
                           L"Error logs OK");
            Serial.clearOutput();

            eventServer.publish(Topic::Disconnected, 24);
            Assert::IsTrue(std::regex_match(
                               Serial.getOutput(),
                               std::regex(R"(\[.*\]\sDisconnected\n)")),
                           L"Disconnected logs OK");

            Serial.clearOutput();

            Assert::AreEqual("", Serial.getOutput());

            eventServer.publish(Topic::Info, 24);
            Assert::IsTrue(std::regex_match(
                               Serial.getOutput(),
                               std::regex(R"(\[.*\]\sInfo:\s24\n)")),
                           L"Info logs long OK");
            Serial.clearOutput();

            log.update(Topic::BatchSize, 24L);

            Assert::IsTrue(std::regex_match(
                               Serial.getOutput(),
                               std::regex(R"(\[.*\]\sTopic\s'1':\s24\n)")), L"Unexpected topic handled OK");
            Serial.clearOutput();
        }
    };
}
