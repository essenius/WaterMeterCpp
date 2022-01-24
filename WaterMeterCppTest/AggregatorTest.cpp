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
#include "../WaterMeterCpp/Aggregator.h"
#include "../WaterMeterCpp/TimeServer.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
    TEST_CLASS(AggregatorTest) {
    public:
        TEST_METHOD(aggregatorFlushTest) {
            EventServer eventServer;
            TimeServer timeServer(&eventServer);
            timeServer.begin();
            RingbufferPayload payload{};
            PayloadBuilder payloadBuilder;
            Serializer serializer(&payloadBuilder);
            DataQueue dataQueue(&eventServer, &serializer);
            Assert::AreEqual(0ULL, payload.timestamp);
            Aggregator aggregator(&eventServer, &dataQueue, &payload);
            aggregator.begin(0);
            Assert::IsFalse(aggregator.newMessage(), L"Can't create new message when flush rate is 0");
            aggregator.setDesiredFlushRate(2);
            Assert::AreEqual(2L, aggregator.getFlushRate(), L"Flush rate OK");
            Assert::IsFalse(aggregator.shouldSend(), L"No need for flush before first message");
            aggregator.newMessage();
            Assert::AreNotEqual(0ULL, payload.timestamp);

            Assert::IsTrue(aggregator.shouldSend(true), L"must send when forced even if not at target size.");
            Assert::IsFalse(aggregator.shouldSend(), L"no need to send before being at target size");
            aggregator.newMessage();
            Assert::IsTrue(aggregator.shouldSend(), L"Must send after two messages");

            // force buffer to look like it's full
            setRingBufferBufferFull(true);
            Assert::IsTrue(aggregator.shouldSend(), L"Should send");
            Assert::IsFalse(aggregator.canSend(), L"Cannot send when buffer full");
            Assert::IsFalse(aggregator.send(), L"Send fails");
            aggregator.newMessage();
            Assert::IsFalse(aggregator.shouldSend(), L"Still can't send when buffer is full");

            // force buffer to look like it got space again
            setRingBufferBufferFull(false);
            Assert::IsFalse(aggregator.shouldSend(), L"Should not send due to not being at multiple of flush rate");
            Assert::IsTrue(aggregator.canSend(), L"Can now send as there is space in the buffer");
            aggregator.newMessage();
            Assert::IsTrue(aggregator.shouldSend(), L"Must send when at multiple of flush rate");

        }
    };
}
