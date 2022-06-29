// Copyright 2022 Rik Essenius
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
#include "TestEventClient.h"
#include "../WaterMeterCpp/EventServer.h"
#include "../WaterMeterCpp/Sampler.h"
#include "../WaterMeterCpp/MagnetoSensorNull.h"

namespace WaterMeterCppTest {

    TEST(SamplerTest, samplerSensorNotFoundTest) {
        EventServer eventServer;
        TestEventClient noSensorClient(&eventServer);
        TestEventClient alertClient(&eventServer);
        eventServer.subscribe(&noSensorClient, Topic::NoSensorFound);
        eventServer.subscribe(&alertClient, Topic::Alert);
        MagnetoSensorReader reader(&eventServer);
        MagnetoSensorNull noSensor;
        MagnetoSensor* list[] = {&noSensor};
        Sampler sampler(&eventServer, &reader, nullptr, nullptr, nullptr, nullptr);
        EXPECT_FALSE(sampler.setup(list, 1)) << L"Setup without a sensor fails";
        EXPECT_EQ(1, noSensorClient.getCallCount()) << "No-sensor event was fired";
        EXPECT_EQ(1, alertClient.getCallCount()) << "Alert fired";
    }

}
