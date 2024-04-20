// Copyright 2022-2024 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#include "gtest/gtest.h"
#include <ESP.h>
// ReSharper disable once CppUnusedIncludeDirective -- false positive
#include <freertos/freeRTOS.h>
#include "TestEventClient.h"
#include "DataQueue.h"
#include "EventServer.h"
#include <SafeCString.h>

#include "Serializer.h"

namespace WaterMeterCppTest {
    using WaterMeter::Clock;
    using WaterMeter::DataQueue;
    using WaterMeter::DataQueuePayload;
    using WaterMeter::SensorSample;
    using WaterMeter::PayloadBuilder;
    using WaterMeter::Serializer;
    using WaterMeter::MaxSamples;

    // ReSharper disable once CyclomaticComplexity -- caused by EXPECT macros

    TEST(DataQueueTest, overrunTest) {
        EventServer eventServer;
        DataQueuePayload payload{};
        DataQueue dataQueue(&eventServer, &payload);
        setRingBufferBufferFull(dataQueue.handle(),true);
        ASSERT_FALSE(dataQueue.send(&payload));
        setRingBufferBufferFull(dataQueue.handle(), false);
        setRingBufferNoMoreEntries(dataQueue.handle());
        ASSERT_FALSE(dataQueue.send(&payload));
    }

    TEST(DataQueueTest, test1) {
        EventServer eventServer;
        Clock theClock(&eventServer);
        TestEventClient resultEventClient(&eventServer);
        TestEventClient sampleEventClient(&eventServer);
        TestEventClient errorEventClient(&eventServer);
        TestEventClient infoEventClient(&eventServer);
        TestEventClient freeSpaceEventClient(&eventServer);

        eventServer.subscribe(&resultEventClient, Topic::Result);
        eventServer.subscribe(&sampleEventClient, Topic::Samples);
        eventServer.subscribe(&errorEventClient, Topic::ConnectionError);
        eventServer.subscribe(&infoEventClient, Topic::Info);
        eventServer.subscribe(&freeSpaceEventClient, Topic::FreeQueueSize);

        // send a number of samples
        PayloadBuilder payloadBuilder;
        Serializer serializer(&eventServer, &payloadBuilder);
        DataQueuePayload payload{};
        DataQueue dataQueue(&eventServer, &payload, 1, 40960, 512, 4096);
        payload.topic = Topic::Samples;

        for (uint16_t i = 0; i < MaxSamples; i++) {
            const auto value = static_cast<int16_t>(i + 475);
            payload.buffer.samples.value[i] = {{value, value}};
        }

        for (uint16_t times = 0; times < 5; times++) {
            payload.buffer.samples.count = MaxSamples - times;
            auto size = DataQueue::requiredSize(payload.size());
            EXPECT_TRUE(size <= 128) << "Payload didn't max out at " << size;
            EXPECT_TRUE(dataQueue.send(&payload)) << "Send works for sample " << times;
            EXPECT_EQ(1, freeSpaceEventClient.getCallCount()) << "Free called right times at " << times;
            EXPECT_STREQ("16790016", freeSpaceEventClient.getPayload()) << "right bytes free at " << times;
            delay(50);
        }

        // send a result
        payload.topic = Topic::Result;
        // clean out buffer to all 0
        for (SensorSample i : payload.buffer.samples.value) {
            i.l = 0;
        }
        payload.buffer.samples.count = 0;

        payload.buffer.result.sampleCount = 81;
        payload.buffer.result.pulseCount = 12;
        payload.buffer.result.skipCount = 2;
        dataQueue.send(&payload);

        // send an error message
        payload.topic = Topic::ConnectionError;
        SafeCString::strcpy(payload.buffer.message, "Not sure what went wrong here...");
        dataQueue.send(&payload);

        // retrieve the samples
        SensorSample expected{{499, 499}};
        for (int i = 0; i < 5; i++) {
            auto payloadReceive = dataQueue.receive();
            EXPECT_NE(nullptr, payloadReceive) << "PayloadReceive not null 1";
            EXPECT_EQ(Topic::Samples, payloadReceive->topic) << "Topic is Samples";
            EXPECT_EQ(expected, payloadReceive->buffer.samples.value[24]) << "Last sample OK";
        }
        EXPECT_EQ(2, freeSpaceEventClient.getCallCount()) << "Free called an extra time";
        EXPECT_STREQ("16789376", freeSpaceEventClient.getPayload()) << "12160 + index #1)";

        // get the result
        auto payloadReceive2 = dataQueue.receive();
        EXPECT_NE(nullptr, payloadReceive2) << "PayloadReceive not null 2";
        EXPECT_EQ(Topic::Result, payloadReceive2->topic) << "Topic 2 is Result";
        EXPECT_EQ(81, payloadReceive2->buffer.result.sampleCount) << "SampleCount=81";
        EXPECT_EQ(12, payloadReceive2->buffer.result.pulseCount) << "PeakCount=12";
        EXPECT_EQ(2, payloadReceive2->buffer.result.skipCount) << "SkipCount=2";

        // get the error message
        auto payloadReceive3 = dataQueue.receive();
        EXPECT_NE(nullptr, payloadReceive3) << "PayloadReceive not null 3";
        EXPECT_EQ(Topic::ConnectionError, payloadReceive3->topic) << "Topic 3 is SamplingError";
        EXPECT_STREQ("Not sure what went wrong here...", payloadReceive3->buffer.message) << "error message ok";
    }

}
