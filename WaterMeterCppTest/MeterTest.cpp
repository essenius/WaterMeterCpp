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

#include "pch.h"

#include <iostream>
#include <regex>

#include <ESP.h>
#include "CppUnitTest.h"
#include "TestEventClient.h"
#include "../WaterMeterCpp/Meter.h"
#include "../WaterMeterCpp/EventServer.h"
#include "../WaterMeterCpp/SafeCString.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
    TEST_CLASS(MeterTest) {
public:
    EventServer eventServer;

    // making sure that the printf redirect works
    TEST_METHOD(meterTest1) {
        Meter meter(&eventServer);
        TestEventClient volumeClient(&eventServer);
        TestEventClient pulseClient(&eventServer);
        eventServer.subscribe(&volumeClient, Topic::Volume);
        eventServer.subscribe(&pulseClient, Topic::Pulses);

        meter.begin();
        eventServer.publish(Topic::SetVolume, "123.4567");
        Assert::AreEqual(1, volumeClient.getCallCount(), L"Volume published");
        Assert::AreEqual("00123.4567000", volumeClient.getPayload(), L"Volume payload returns initial value");
        Assert::AreEqual(1, pulseClient.getCallCount(), L"Pulses published");
        Assert::AreEqual("0", pulseClient.getPayload(), L"Pulse payload is 0");

        const char* expected[] = { "00123.4567001", "00123.4567001", "00123.4567002", "00123.4567003", "00123.4567003",
                                   "00123.4567004", "00123.4567004", "00123.4567005", "00123.4567006", "00123.4567006" };
        for (int i = 0; i < std::size(expected); i++) {
            volumeClient.reset();
            pulseClient.reset();
            eventServer.publish(Topic::Peak, LONG_TRUE);
            char buffer[10];
            safeSprintf(buffer, "%d", i + 1);

            Assert::AreEqual(1, volumeClient.getCallCount(), L"Volume published");
            Assert::AreEqual(expected[i], volumeClient.getPayload(), buffer);
            Assert::AreEqual(1, pulseClient.getCallCount(), L"Pulses published");
            Assert::AreEqual(buffer, pulseClient.getPayload(), L"Pulse payload is correct");
        }

        Assert::IsFalse(meter.setVolume("x"), L"non-double not accepted");
        Assert::AreEqual(expected[std::size(expected) - 1], meter.getVolume(), L"Value not changed");

    }
    };
}