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
#include "TestEventClient.h"
#include "../WaterMeterCpp/SampleAggregator.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
    TEST_CLASS(SampleAggregatorTest) {
    public:
        TEST_METHOD(sampleAggregatorAddSampleTest) {
            EventServer eventServer;
            RingbufferPayload payload{};
            PayloadBuilder payloadBuilder;
            Serializer serializer(&payloadBuilder);
            Clock theClock(&eventServer);
            DataQueue dataQueue(&eventServer, &theClock, &serializer);

            SampleAggregator aggregator(&eventServer, &theClock, &dataQueue, &payload);
            aggregator.begin();
            Assert::AreEqual(50L, aggregator.getFlushRate(), L"Default flush rate OK");
            eventServer.publish(Topic::BatchSizeDesired, "5");
            Assert::AreEqual(5L, aggregator.getFlushRate(), L"Flush rate changed");
            eventServer.publish(Topic::BatchSizeDesired, "DEFAULT");
            Assert::AreEqual(50L, aggregator.getFlushRate(), L"Flush rate changed back to default");
            eventServer.publish(Topic::BatchSizeDesired, 2);
            Assert::AreEqual(2L, aggregator.getFlushRate(), L"Flush rate changed");
            aggregator.flush();
            aggregator.addSample(1000);
            Assert::IsFalse(aggregator.shouldSend());
            Assert::AreEqual(1U, static_cast<unsigned>(payload.buffer.samples.count), L"One sample added");
            Assert::AreEqual<int16_t>(1000, payload.buffer.samples.value[0], L"First sample value correct");

            aggregator.addSample(-1000);
            Assert::IsTrue(aggregator.shouldSend(), L"Needs flush after two measurements");

            // specialization for uint16_t does not work for some reason
            Assert::AreEqual(2U, static_cast<unsigned>(payload.buffer.samples.count), L"Second sample added");
            Assert::AreEqual<int16_t>(1000, payload.buffer.samples.value[0], L"First sample value still correct");
            Assert::AreEqual<int16_t>(-1000, payload.buffer.samples.value[1], L"Second sample value correct");

            aggregator.flush();
            Assert::AreEqual(0U, static_cast<unsigned>(payload.buffer.samples.count), L"Buffer empty after flush");

        }

        TEST_METHOD(sampleAggregatorZeroFlushRateTest) {
            EventServer eventServer;
            Clock theClock(&eventServer);
            RingbufferPayload payload{};
            PayloadBuilder payloadBuilder;
            Serializer serializer(&payloadBuilder);
            DataQueue dataQueue(&eventServer, &theClock, &serializer);

            TestEventClient batchSizeListener(&eventServer);
            eventServer.subscribe(&batchSizeListener, Topic::BatchSize);
            SampleAggregator aggregator(&eventServer, &theClock, &dataQueue, &payload);
            aggregator.begin();

            Assert::IsFalse(aggregator.send());

            Assert::AreEqual(1, batchSizeListener.getCallCount(), L"batch size set");
            Assert::AreEqual("50", batchSizeListener.getPayload(), L"batch size is 50");
            Assert::AreEqual(50L, aggregator.getFlushRate(), L"Default flush rate OK");

            batchSizeListener.reset();
            eventServer.publish(Topic::BatchSizeDesired, 2L);
            Assert::AreEqual(1, batchSizeListener.getCallCount(), L"batch size changed (no measurements yet)");
            Assert::AreEqual("2", batchSizeListener.getPayload(), L"batch size is 2");

            batchSizeListener.reset();
            aggregator.addSample(1000);

            Assert::IsFalse(aggregator.send(), L"No need to send after 1 measurement");

            // -1 should clip to 0;
            eventServer.publish(Topic::BatchSizeDesired, -1L);
            Assert::AreEqual(0, batchSizeListener.getCallCount(), L"batch size not changed");
            Assert::AreEqual(2L, aggregator.getFlushRate(), L"Flush rate not changed");
            aggregator.addSample(3000);
            Assert::IsTrue(aggregator.shouldSend(), L"Must send after two measurements");
            const auto currentTimestamp = payload.timestamp;
            Assert::AreNotEqual(0ULL, currentTimestamp, L"Timestamp set");
            Assert::AreEqual(static_cast<int16_t>(1000), payload.buffer.samples.value[0], L"First value OK");
            Assert::AreEqual(static_cast<int16_t>(3000), payload.buffer.samples.value[1], L"Second value OK");

            Assert::IsTrue(aggregator.send(), L"Send successful");

            Assert::AreEqual(0L, aggregator.getFlushRate(), L"Flush rate changed");
            aggregator.flush();
            aggregator.addSample(5000);
            Assert::IsFalse(aggregator.shouldSend(), L"No need to send");
            Assert::AreNotEqual(currentTimestamp, payload.timestamp, L"Timestamp set");
            Assert::AreEqual(0U, static_cast<unsigned>(payload.buffer.samples.count), L"Buffer empty");

            // check whether failure to write is handled OK
            eventServer.publish(Topic::BatchSizeDesired, 2L);
            Assert::AreEqual(2L, aggregator.getFlushRate(), L"Flush rate changed back to 2");
            aggregator.addSample(-2000);
            setRingBufferBufferFull(true);
            aggregator.addSample(-3000);
            Assert::AreEqual(0U, static_cast<unsigned>(payload.buffer.samples.count), L"Buffer flushed since we can't write");


            // reconnect
            setRingBufferBufferFull(false);
            aggregator.addSample(-4000);
            Assert::AreEqual(1U, static_cast<unsigned>(payload.buffer.samples.count), L"restarted filling buffer");

            Assert::IsFalse(aggregator.shouldSend(), L"No flush needed after first");

            // Switch to max buffer size 
            batchSizeListener.reset();
            eventServer.publish(Topic::BatchSizeDesired, 10000L);
            aggregator.addSample(-5000);
            Assert::IsTrue(aggregator.send(), L"sends after reconnect");

            Assert::AreEqual(1, batchSizeListener.getCallCount(), L"batch size listener called once");
            Assert::AreEqual("50", batchSizeListener.getPayload(), L"payload maximizet at 50");
        }
    };
}
