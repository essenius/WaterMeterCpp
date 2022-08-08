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

#include <regex>
#include <ESP.h>

#include "TestEventClient.h"
#include "../WaterMeterCpp/Meter.h"
#include "../WaterMeterCpp/EventServer.h"
#include "../WaterMeterCpp/SafeCString.h"

namespace WaterMeterCppTest {
    
    class MeterTest : public testing::Test {
    public:
        EventServer eventServer;
    };

    // making sure that the printf redirect works
    TEST_F(MeterTest, meterTest1) {
        Meter meter(&eventServer);
        TestEventClient volumeClient(&eventServer);
        TestEventClient pulseClient(&eventServer);
        eventServer.subscribe(&volumeClient, Topic::Volume);
        eventServer.subscribe(&pulseClient, Topic::Pulses);

        meter.begin();
        eventServer.publish(Topic::SetVolume, "123.4567");
        EXPECT_EQ(1, volumeClient.getCallCount()) << "Volume published";
        EXPECT_STREQ("00123.4567000", volumeClient.getPayload()) << L"Volume payload returns initial value";
        EXPECT_EQ(1, pulseClient.getCallCount()) << "Pulses published";
        EXPECT_STREQ("0", pulseClient.getPayload()) << L"Pulse payload is 0";

        const char* expected[] = {
            "00123.4567303", "00123.4567606", "00123.4567909", "00123.4568212", "00123.4568515",
            "00123.4568818", "00123.4569121", "00123.4569424", "00123.4569727", "00123.4570030"
        };
        for (unsigned int i = 0; i < std::size(expected); i++) {
            volumeClient.reset();
            pulseClient.reset();
            eventServer.publish(Topic::Pulse, i*2 + 1);
            char buffer[10];
            safeSprintf(buffer, "%d", i + 1);

            EXPECT_EQ(1, volumeClient.getCallCount()) << "Volume published";
            EXPECT_STREQ(expected[i], volumeClient.getPayload()) << buffer;
            EXPECT_EQ(1, pulseClient.getCallCount()) << "Pulses published";
            EXPECT_STREQ(buffer, pulseClient.getPayload()) << "Pulse payload is correct";
        }

        EXPECT_FALSE(meter.setVolume("x")) << "non-double not accepted";
        EXPECT_STREQ(expected[std::size(expected) - 1], meter.getVolume()) << "Value not changed";
    }
}
