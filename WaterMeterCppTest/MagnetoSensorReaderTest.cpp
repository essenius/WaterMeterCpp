// Copyright 2021-2022 Rik Essenius
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

#include "CppUnitTest.h"
#include "TestEventClient.h"
#include "Wire.h"
#include "../WaterMeterCpp/MagnetoSensorReader.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// doesn't do much, just checking the interface works

namespace WaterMeterCppTest {
    TEST_CLASS(MagnetoSensorReaderTest) {
    public:
        TEST_METHOD(magnetoSensorReaderTest1) {
            MagnetoSensor sensor;
            EventServer eventServer;
            TestEventClient resetSensorEventClient(&eventServer);
            TestEventClient alertEventClient(&eventServer);
            eventServer.subscribe(&resetSensorEventClient, Topic::SensorWasReset);
            eventServer.subscribe(&alertEventClient, Topic::Alert);
            MagnetoSensorReader reader(&eventServer, &sensor);
            Wire.setFlatline(true);
            reader.begin();
            Wire.setEndTransmissionTogglePeriod(10);

            for (int streaks = 0; streaks < 10; streaks++) {
                for (int sample = 0; sample < 100; sample++) {
                    reader.read();
                    Assert::AreEqual(streaks, resetSensorEventClient.getCallCount(), L"right number of events fired");
                    Assert::AreEqual(0, alertEventClient.getCallCount(), L"Alert event not fired");
                }
            }
            reader.read();
            Assert::AreEqual(10, resetSensorEventClient.getCallCount(), L"ResetSensor event fired 10 times");
            Assert::AreEqual(1, alertEventClient.getCallCount(), L"Alert event fired");

            resetSensorEventClient.reset();
            eventServer.publish(Topic::ResetSensor, LONG_TRUE);
            Assert::AreEqual(
                1,
                resetSensorEventClient.getCallCount(),
                L"Sensor was softReset called after ResetSensor command");
            Wire.setEndTransmissionTogglePeriod(0);
            constexpr uint8_t BUFFER[] = {10, 0x80, 11, 0x01, 9, 0x19};
            Assert::AreEqual<short>(sizeof BUFFER, Wire.writeMismatchIndex(BUFFER, sizeof BUFFER), L"test");
        }
    };
}
