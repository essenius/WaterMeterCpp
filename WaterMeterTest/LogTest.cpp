﻿// Copyright 2021-2024 Rik Essenius
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
#include "Log.h"
#include "EventServer.h"
#include "Clock.h"
#include "ConnectionState.h"

namespace WaterMeterCppTest {
    using WaterMeter::Clock;
    using WaterMeter::ConnectionState;
    using WaterMeter::EventServer;
    using WaterMeter::Log;
    using WaterMeter::PayloadBuilder;
    using WaterMeter::Topic;
    using WaterMeter::SensorState;
    
    class LogTest : public testing::Test {
    public:
        EventServer eventServer;
    protected:
        void publishConnectionState(const Topic topic, ConnectionState connectionState) {
            eventServer.publish(topic, static_cast<long>(connectionState));
        }
    };

    TEST_F(LogTest, printfTest) {
        // making sure that the printf redirect works
        clearPrintOutput();
        redirectPrintf("Hello %s\n", "there");
        EXPECT_STREQ("Hello there\n", getPrintOutput()) << "Output OK";
        clearPrintOutput();
        EXPECT_EQ(0, strlen(getPrintOutput())) << "Zero length";
    }

    TEST_F(LogTest, scriptTest) {
        PayloadBuilder payloadBuilder;
        Log log(&eventServer, &payloadBuilder);
        Clock theClock(&eventServer);
        theClock.begin();
        log.begin();

        clearPrintOutput();
        publishConnectionState(Topic::Connection, ConnectionState::MqttReady);

        // the pattern we expect here is [2022-02-22T01:02:03.456789] MQTT Ready\n
        const auto regexPattern = R"(\[\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{6}Z\]\sMQTT ready\n)";
        EXPECT_TRUE(std::regex_match(getPrintOutput(), std::regex(regexPattern))) << "Log pattern matches";

        // switch off time stamp generation for the rest of the tests
        eventServer.cannotProvide(&theClock, Topic::Time);

        clearPrintOutput();
        eventServer.publish(Topic::MessageFormatted, "My Message");
        EXPECT_STREQ("[] My Message\n", getPrintOutput()) << "Message logs OK";

        clearPrintOutput();
        publishConnectionState(Topic::Connection, ConnectionState::Disconnected);
        EXPECT_STREQ("[] Disconnected\n", getPrintOutput()) << "Disconnected logs OK";

        clearPrintOutput();
        eventServer.publish(Topic::MessageFormatted, 24);
        EXPECT_STREQ("[] 24\n", getPrintOutput()) << "MessageFormatted accepts long OK";

        clearPrintOutput();
        log.update(Topic::BatchSize, 24L);
        EXPECT_STREQ("[] Topic '1': 24\n", getPrintOutput()) << "Unexpected topic handled OK";

        clearPrintOutput();
        eventServer.publish(Topic::Anomaly, static_cast<uint16_t>(SensorState::Outlier) + (1234 << 4));
        EXPECT_STREQ("[] Anomaly: Outlier (12.34)\n", getPrintOutput()) << "Anomaly handled OK";

        clearPrintOutput();
        eventServer.publish(Topic::TimeOverrun, 1234);
        EXPECT_STREQ("[] Time overrun: 1234\n", getPrintOutput()) << "Time overrun handled OK";

        clearPrintOutput();
        eventServer.publish(Topic::Blocked, true);
        EXPECT_STREQ("[] Blocked: 1\n", getPrintOutput()) << "Blocked handled OK";

        clearPrintOutput();
        eventServer.publish(Topic::SensorWasReset, 1);
        EXPECT_STREQ("[] Sensor was soft-reset\n", getPrintOutput()) << "Sensor soft reset handled OK";

        clearPrintOutput();
        eventServer.publish(Topic::SensorWasReset, 2);
        EXPECT_STREQ("[] Sensor was hard-reset\n", getPrintOutput()) << "Sensor hard reset handled OK";

        clearPrintOutput();
        eventServer.publish(Topic::FreeQueueSpaces, 0x03000010);
        EXPECT_STREQ("[] Free Spaces Queue #3: 16\n", getPrintOutput()) << "Sensor queue spaces handled OK";

        clearPrintOutput();
        eventServer.publish(Topic::SensorState, true);
        EXPECT_STREQ("[] Sensor state: Ok\n", getPrintOutput()) << "no sensor found handled OK";

        clearPrintOutput();
        eventServer.publish(Topic::NoDisplayFound, true);
        EXPECT_STREQ("[] No OLED display found\n", getPrintOutput()) << "no display found handled OK";
        clearPrintOutput();

        eventServer.publish(Topic::UpdateProgress, 73);
        EXPECT_STREQ("[] Firmware update progress: 73%\n", getPrintOutput()) << "Update progress logged OK";
        clearPrintOutput();

        eventServer.publish(Topic::NoFit, 55);
        EXPECT_STREQ("[] No fit: 55 deg\n", getPrintOutput()) << "NoFit logged OK";
        clearPrintOutput();
    }
}
