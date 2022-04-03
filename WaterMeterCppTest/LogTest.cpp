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

#include "pch.h"

#include <iostream>
#include <regex>

#include <ESP.h>
#include "CppUnitTest.h"
#include "../WaterMeterCpp/Log.h"
#include "../WaterMeterCpp/EventServer.h"
#include "../WaterMeterCpp/Clock.h"
#include "../WaterMeterCpp/ConnectionState.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
    TEST_CLASS(LogTest) {
    public:
        EventServer eventServer;

        // making sure that the printf redirect works
        TEST_METHOD(logPrintfTest) {
            redirectPrintf("Hello %s\n", "there");
            Assert::AreEqual("Hello there\n", getPrintOutput());
            clearPrintOutput();
            Assert::AreEqual<size_t>(0, getPrintOutputLength());
        }

        TEST_METHOD(logScriptTest) {
            PayloadBuilder payloadBuilder;
            Log log(&eventServer, &payloadBuilder);
            Clock theClock(&eventServer);
            theClock.begin();
            log.begin();

            clearPrintOutput();
            publishConnectionState(Topic::Connection, ConnectionState::MqttReady);

            // the pattern we expect here is [2022-02-22T01:02:03.456789] MQTT Ready\n
            Assert::IsTrue(
                std::regex_match(
                    getPrintOutput(),
                    std::regex(R"(\[\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{6}\]\sMQTT ready\n)")),
                L"Log pattern matches");

            // switch off time stamp generation for the rest of the tests
            eventServer.cannotProvide(&theClock, Topic::Time);

            clearPrintOutput();
            eventServer.publish(Topic::MessageFormatted, "My Message");
            Assert::AreEqual("[] My Message\n", getPrintOutput(), L"Message logs OK");

            clearPrintOutput();
            publishConnectionState(Topic::Connection, ConnectionState::Disconnected);
            Assert::AreEqual("[] Disconnected\n", getPrintOutput(), L"Disconnected logs OK");

            clearPrintOutput();
            Assert::AreEqual("", getPrintOutput());

            eventServer.publish(Topic::MessageFormatted, 24);
            Assert::AreEqual("[] 24\n", getPrintOutput(), L"MessageFormatted accepts long OK");

            clearPrintOutput();
            log.update(Topic::BatchSize, 24L);
            Assert::AreEqual("[] Topic '1': 24\n", getPrintOutput(), L"Unexpected topic handled OK");

            clearPrintOutput();
            eventServer.publish(Topic::Alert, 1);
            Assert::AreEqual("[] Alert: 1\n", getPrintOutput(), L"Alert handled OK");

            clearPrintOutput();
            eventServer.publish(Topic::TimeOverrun, 1234);
            Assert::AreEqual("[] Time overrun: 1234\n", getPrintOutput(), L"Time overrun handled OK");

            clearPrintOutput();
            eventServer.publish(Topic::ResultWritten, LONG_TRUE);
            Assert::AreEqual("[] Result Written: 1\n", getPrintOutput(), L"Result Written handled OK");

            clearPrintOutput();
            eventServer.publish(Topic::Blocked, LONG_TRUE);
            Assert::AreEqual("[] Blocked: 1\n", getPrintOutput(), L"Blocked handled OK");

            clearPrintOutput();
            eventServer.publish(Topic::SensorWasReset, 3);
            Assert::AreEqual("[] Sensor was reset: 3\n", getPrintOutput(), L"Sensor reset handled OK");

            clearPrintOutput();
            eventServer.publish(Topic::FreeQueueSpaces, 0x03000010);
            Assert::AreEqual("[] Free Spaces Queue #3: 16\n", getPrintOutput(), L"Sensor reset handled OK");

            clearPrintOutput();
            eventServer.publish(Topic::NoSensorFound, LONG_TRUE);
            Assert::AreEqual("[] No sensor found\n", getPrintOutput(), L"no sensor found handled OK");
        }

    private:
        void publishConnectionState(const Topic topic, ConnectionState connectionState) {
            eventServer.publish(topic, static_cast<long>(connectionState));
        }
    };
}
