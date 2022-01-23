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
#include "CppUnitTest.h"
#include <regex>

#include "TimeServerMock.h"
#include "../WaterMeterCpp/ArduinoMock.h"
#include "../WaterMeterCpp/EventServer.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
    TEST_CLASS(TimeServerTest) {
    public:
        TEST_METHOD(timeServerScriptTest) {
            EventServer eventServer;
            // the mock only mocks the time setting and detection, but keeps the rest
            TimeServerMock timeServer(&eventServer);

            Assert::IsFalse(timeServer.timeWasSet(), L"Time was not set");

            timeServer.begin();
            Assert::IsFalse(timeServer.timeWasSet(), L"Time was not set");
            timeServer.setTime();
            Assert::IsTrue(timeServer.timeWasSet(), L"Time was set");

            // trigger the get function with Topic::Time
            const char* timestamp = eventServer.request(Topic::Time, "");
            Assert::AreEqual(std::size_t{26}, strlen(timestamp), L"Length of timestamp ok");
            Assert::IsTrue(
                std::regex_match(timestamp, std::regex(R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{6})")),
                L"Time pattern matches");

            timestamp = timeServer.get(Topic::BatchSize, nullptr);
            Assert::IsNull(timestamp, L"Unexpected topic returns default");

            eventServer.cannotProvide(&timeServer);
            timestamp = eventServer.request(Topic::Time, "");
            Assert::AreEqual("", timestamp, "Time no longer available");

            eventServer.provides(&timeServer, Topic::Time);
            timestamp = eventServer.request(Topic::Time, "");
            Assert::AreNotEqual("", timestamp, "Time filled again");

            eventServer.cannotProvide(&timeServer, Topic::Time);
            timestamp = eventServer.request(Topic::Time, "");
            Assert::AreEqual("", timestamp, "Time no longer available");
        }

        TEST_METHOD(timeServerTimeWasSetTest) {
            EventServer eventServer;
            TimeServer timeServer(&eventServer);
            timeServer.begin();
            Assert::IsTrue(timeServer.timeWasSet());
        }
    };
}
