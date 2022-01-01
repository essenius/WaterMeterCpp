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
#include "../WaterMeterCpp/BatchWriter.h"
#include "../WaterMeterCpp/TimeServer.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
    TEST_CLASS(BatchWriterTest) {
    public:
        TEST_METHOD(batchWriterWriteParamTest) {
            EventServer eventServer;
            TimeServer timeServer(&eventServer);
            timeServer.begin();
            PayloadBuilder builder;
            BatchWriter bw("BW", &eventServer, &builder);
            bw.begin(0);
            Assert::IsFalse(bw.newMessage(), L"Can't create new message when flush rate is 0");
            bw.setDesiredFlushRate(5);
            Assert::AreEqual(5L, bw.getFlushRate(), L"Flush rate OK");
            Assert::IsFalse(bw.needsFlush(), L"No need for flush before first message");
            bw.newMessage();
            Assert::IsTrue(
                std::regex_match(
                    bw.getMessage(),
                    std::regex(R"(\{"timestamp":"\d{4}\-\d{2}\-\d{2}T\d{2}:\d{2}:\d{2}\.\d{6}")")
                ), L"Start of message created correctly");

            Assert::IsFalse(bw.needsFlush(), L"Still no need for flush after new message");
            Assert::IsTrue(bw.needsFlush(true), L"Needs flush when at end. This also kicks off prep");
            Assert::IsTrue(bw.needsFlush(), L"Needs flush after flush preparation");
            Assert::IsTrue(
                std::regex_match(
                    bw.getMessage(),
                    std::regex(R"(\{"timestamp":"\d{4}\-\d{2}\-\d{2}T\d{2}:\d{2}:\d{2}\.\d{6}"\})")
                ), L"Message created correctly");
        }
    };
}
