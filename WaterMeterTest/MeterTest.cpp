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

#include <regex>
#include <ESP.h>

#include "TestEventClient.h"
#include "Meter.h"
#include "EventServer.h"
#include <SafeCString.h>

namespace WaterMeterCppTest {
    using WaterMeter::Meter;

    class MeterTest : public testing::Test {
    public:
        EventServer eventServer;
    };

    // making sure that the printf redirect works
    TEST_F(MeterTest, scriptTest) {
        Meter meter(&eventServer);
        TestEventClient volumeClient(&eventServer);
        TestEventClient pulseClient(&eventServer);
        TestEventClient meterClient(&eventServer);
        eventServer.subscribe(&volumeClient, Topic::Volume);
        eventServer.subscribe(&pulseClient, Topic::Pulses);
        eventServer.subscribe(&meterClient, Topic::MeterPayload);

        meter.begin();
        eventServer.publish(Topic::SetVolume, "0.4567");
        EXPECT_EQ(1, volumeClient.getCallCount()) << "Volume published";
        EXPECT_STREQ("00000.4567000", volumeClient.getPayload()) << L"Volume payload returns initial value";
        EXPECT_STREQ(R"({"timestamp":"","pulses":0,"volume":00000.4567000})", meterClient.getPayload()) << L"Volume payload returns initial value";
        EXPECT_EQ(1, pulseClient.getCallCount()) << "Pulses published";
        EXPECT_STREQ("0", pulseClient.getPayload()) << L"Pulse payload is 0";

        eventServer.publish(Topic::AddVolume, R"({"timestamp":,"pulses":0,"volume":00123.0000000})");
        EXPECT_EQ(2, volumeClient.getCallCount()) << "Volume published";
        EXPECT_STREQ("00123.4567000", volumeClient.getPayload()) << L"Volume payload returns sum of published and kept value";
        EXPECT_EQ(2, pulseClient.getCallCount()) << "Pulses published";
        EXPECT_STREQ("0", pulseClient.getPayload()) << L"Pulse payload is 0";

        const char* expected[] = {
            "00123.4567609", "00123.4568217", "00123.4568826","00123.4569434", "00123.4570043",
            "00123.4570651", "00123.4571260", "00123.4571868", "00123.4572477", "00123.4573085"
        };

        for (unsigned int i = 0; i < std::size(expected); i++) {
            volumeClient.reset();
            pulseClient.reset();
            eventServer.publish(Topic::Pulse, i*2 + 1);
            char pulseBuffer[10];
            SafeCString::sprintf(pulseBuffer, "%d", i + 1);

            EXPECT_EQ(1, volumeClient.getCallCount()) << "Volume published";
            EXPECT_STREQ(expected[i], volumeClient.getPayload()) << "Volume payload for " << pulseBuffer;
            EXPECT_EQ(1, pulseClient.getCallCount()) << "Pulses published";
            EXPECT_STREQ(pulseBuffer, pulseClient.getPayload()) << "Pulse payload is correct";
        }

        EXPECT_FALSE(meter.setVolume("x")) << "non-double not accepted";
        EXPECT_STREQ(expected[std::size(expected) - 1], meter.getVolume()) << "Value not changed";
    }
}
