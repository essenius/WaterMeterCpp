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

#include <MagnetoSensorNull.h>

#include "FlowDetectorDriver.h"
#include "MagnetoSensorMock.h"
#include "SamplerDriver.h"
#include "gtest/gtest.h"
#include "TestEventClient.h"
#include "../WaterMeter/EventServer.h"
#include "../WaterMeter/Sampler.h"

namespace WaterMeterCppTest {

    TEST(SamplerTest, samplerSensorNotFoundTest) {
        EventServer eventServer;
        TestEventClient noSensorClient(&eventServer);
        eventServer.subscribe(&noSensorClient, Topic::SensorState);
        MagnetoSensorReader reader(&eventServer);
        MagnetoSensorNull noSensor;
        MagnetoSensor* list[] = {&noSensor};
        digitalWrite(MagnetoSensorReader::DefaultPowerPort, LOW);

        ChangePublisher<uint8_t> buttonPublisher(&eventServer, Topic::ResetSensor);
        Button button(&buttonPublisher, 34);
        Sampler sampler(&eventServer, &reader, nullptr, &button, nullptr, nullptr, nullptr);
        EXPECT_FALSE(sampler.begin(list, 1)) << L"Setup without a sensor fails";
        EXPECT_EQ(1, noSensorClient.getCallCount()) << "No-sensor event was fired";
    }

    TEST(SamplerTest, SamplerOverrunTest) {
        EventServer eventServer;
        TestEventClient overrunClient(&eventServer);
        eventServer.subscribe(&overrunClient, Topic::TimeOverrun);
        MagnetoSensorReader reader(&eventServer);
        ChangePublisher<uint8_t> buttonPublisher(&eventServer, Topic::ResetSensor);
        MagnetoSensorMock mockSensor;
        MagnetoSensor* list[] = { &mockSensor };
        FlowDetectorDriver flowDetector(&eventServer, nullptr);
        DataQueuePayload payload1;
        DataQueue dataQueue1(&eventServer, &payload1);
        DataQueue dataQueue2(&eventServer, &payload1);
        DataQueuePayload payload2;
        DataQueuePayload payload3;
        SampleAggregator sampleAggregator(&eventServer, nullptr, &dataQueue1, &payload2);
        ResultAggregator resultAggregator(&eventServer, nullptr, &dataQueue2, &payload3, 10000);
        QueueClient queueClient(&eventServer, nullptr, 10, 0);
        Button button(&buttonPublisher, 34);
        SamplerDriver sampler(&eventServer, &reader, &flowDetector, &button, &sampleAggregator, &resultAggregator, &queueClient);

        EXPECT_TRUE(sampler.begin(list, 1)) << "Begin with mock sensor succeeds";
        auto handle = reinterpret_cast<TaskHandle_t>(3);
        sampler.beginLoop(handle);
        // emulate timer, sample coming in, handling
        SamplerDriver::onTimer();
        sampler.sensorLoop();
        sampler.loop();
        EXPECT_EQ(0, overrunClient.getCallCount()) << "No overrun in first round";
        SamplerDriver::onTimer();
        delay(15);
        sampler.sensorLoop();
        sampler.loop();
        EXPECT_EQ(1, overrunClient.getCallCount()) << "Overrun in second round";
        EXPECT_STREQ("5250", overrunClient.getPayload()) << "Overrun OK: 10ms normal, 10ms max, 5ms too much at start, 50ns added";

        SamplerDriver::onTimer();
        sampler.sensorLoop();
        sampler.loop();
        EXPECT_EQ(2, overrunClient.getCallCount()) << "Overrun reset in third round";
        EXPECT_STREQ("0", overrunClient.getPayload()) << "Overrun back to 0";
    }

/*    aggregator.begin();
    eventServer.publish(Topic::IdleRate, 1);
    eventServer.publish(Topic::NonIdleRate, 1);
    constexpr Coordinate AVERAGE{2400, 2400};
    EllipseFit ellipseFit;

    const FlowDetectorDriver fmd(&eventServer, &ellipseFit, AVERAGE);
    aggregator.addMeasurement(IntCoordinate{{2398, 0}}, &fmd);
    eventServer.publish(Topic::ProcessTime, 10125L);
    EXPECT_TRUE(aggregator.shouldSend()) << "Needs flush";
    const auto result = &payload.buffer.result;

    // 1 point written, flush rate 1, measures contains sensor value, overrun triggered.
    assertSummary(2398, 1, 0, 0, result);
    assertExceptions(0, 1, result);
    assertDuration(10125, 10125, 10125, result);
    // TODO: filtered value analysis
}*/

}
