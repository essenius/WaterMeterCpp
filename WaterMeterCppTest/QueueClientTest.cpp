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

#include <CppUnitTest.h>

#include "../WaterMeterCpp/QueueClient.h"
#include "StateHelper.h"
#include "TestEventClient.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
    TEST_CLASS(QueueClientTest) {
    public:
        static EventServer eventServer;
        static TestEventClient testEventClient;

        TEST_METHOD(queueClientTest1) {
            uxQueueReset();
            eventServer.subscribe(&testEventClient, Topic::Exclude);
            constexpr uint16_t QUEUE_SIZE = 10;
            QueueClient qClient(&eventServer, QUEUE_SIZE);
            qClient.begin(qClient.getQueueHandle());
            eventServer.subscribe(&qClient, Topic::Exclude);
            for (int i = 0; i < QUEUE_SIZE; i++) {
                eventServer.publish(Topic::Exclude, i * 11);
            }
            // should not get saved
            eventServer.publish(Topic::Exclude, "111");
            testEventClient.reset();
            for (int i = 0; i < QUEUE_SIZE; i++) {
                char numbuf[10];
                Assert::IsTrue(qClient.receive(), L"receive true");
                Assert::AreEqual(i + 1, testEventClient.getCallCount(), L"Call count");
                safeSprintf(numbuf, "%d", 11 * i);
                Assert::AreEqual(numbuf, testEventClient.getPayload(), L"Payload");
            }
            Assert::IsFalse(qClient.receive(), L"Receive false");
            Assert::AreEqual<int>(QUEUE_SIZE, testEventClient.getCallCount(), L"Call count last");
        }
    };

    EventServer QueueClientTest::eventServer;
    TestEventClient QueueClientTest::testEventClient(&eventServer);

}
