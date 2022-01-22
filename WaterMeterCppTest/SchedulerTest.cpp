// Copyright 2021 Rik Essenius
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

/*
#include "CppUnitTest.h"
#include "../WaterMeterCpp/Scheduler.h"
#include "../WaterMeterCpp/MeasurementAggregator.h"
#include "../WaterMeterCpp/ResultAggregator.h"
#include "../WaterMeterCpp/ArduinoMock.h"

#include "FlowMeterDriver.h"
#include "TestEventClient.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

constexpr int MEASURE_INTERVAL_MICROS = 10000;

namespace WaterMeterCppTest {


    TEST_CLASS(SchedulerTest) {
    public:
        static EventServer eventServer;
        static TestEventClient MeasurementListener;
        static TestEventClient ResultListener;
        //static PayloadBuilder Builder1, Builder2;
        static RingbufferPayload payload1, payload2;
        static MeasurementAggregator MeasurementWriter;
        static ResultAggregator ResultWriter;
        static PayloadBuilder payloadBuilder;
        static Scheduler Scheduler;

        static Serializer serializer;
        static RingbufHandle_t ringbufHandle;
        static DataQueue dataQueue;

        TEST_CLASS_INITIALIZE(schedulerClassInitalize) {
            eventServer.subscribe(&MeasurementListener, Topic::Measurement);
            eventServer.subscribe(&ResultListener, Topic::Result);
            ringbufHandle = xRingbufferCreate(40960, RINGBUF_TYPE_ALLOWSPLIT);
            
        }

        TEST_METHOD_INITIALIZE(schedulerMethodInitialize) {
            MeasurementWriter.begin();
            ResultWriter.begin();
            MeasurementListener.reset();
            ResultListener.reset();
        }

        TEST_METHOD(schedulerForcedDoubleWriteTest) {
            eventServer.publish(Topic::BatchSizeDesired, 1);
            eventServer.publish(Topic::IdleRate, 1);

            registerMeasurement(1000, 350);
            Scheduler.processOutput();
            Assert::AreEqual(1, MeasurementListener.getCallCount(), L"Measurement was sent");
            Assert::AreEqual(R"({"timestamp":"","measurements":[1000]})", MeasurementListener.getPayload(),
                             L"Payload OK");
            Assert::AreEqual(1, ResultListener.getCallCount(), L"Measurement was sent");
            Assert::AreEqual(
                R"({"timestamp":"","lastValue":1000,"summaryCount":{"samples":1,"peaks":0,"flows":0,"maxStreak":1},)"
                R"("exceptionCount":{"outliers":0,"excludes":0,"overruns":0},)"
                R"("duration":{"total":350,"average":350,"max":350},)"
                R"("analysis":{"smoothValue":1000,"derivative":0,"smoothDerivative":0,"smoothAbsDerivative":0}})",
                ResultListener.getPayload(),
                L"Payload OK");
        }

        TEST_METHOD(schedulerAlternateWriteTest) {
            eventServer.publish(Topic::BatchSizeDesired, 2);
            Assert::AreEqual(2L, MeasurementWriter.getFlushRate(), L"measurement flush rate is 2");
            Assert::AreNotEqual(2L, ResultWriter.getFlushRate(), L"result flush rate is not 2");

            eventServer.publish(Topic::IdleRate, 2);
            Assert::AreEqual(2L, ResultWriter.getFlushRate(), L"result flush rate is 2");

            eventServer.publish(Topic::NonIdleRate, 33);
            Assert::AreEqual(2L, MeasurementWriter.getFlushRate(), L"measurement flush rate is still 2");
            Assert::AreEqual(2L, ResultWriter.getFlushRate(), L"result flush rate is still 2");

            registerMeasurement(1100, 400);
            Scheduler.processOutput();
            Assert::AreEqual(0, MeasurementListener.getCallCount(), L"Measurement was not sent");
            Assert::AreEqual(0, ResultListener.getCallCount(), L"Result was not sent");

            registerMeasurement(1105, 405);
            Scheduler.processOutput();
            Assert::AreEqual(1, MeasurementListener.getCallCount(), L"Measurement was sent");
            Assert::AreEqual(0, ResultListener.getCallCount(), L"Result was not sent");
            Assert::AreEqual(R"({"timestamp":"","measurements":[1100,1105]})", MeasurementListener.getPayload(),
                             "measurement ok");

            registerMeasurement(1103, 410);
            Scheduler.processOutput();
            Assert::AreEqual(1, MeasurementListener.getCallCount(), L"New measurement was not sent");
            Assert::AreEqual(1, ResultListener.getCallCount(), L"Result was sent");
            Assert::AreEqual(
                R"({"timestamp":"","lastValue":1105,"summaryCount":{"samples":2,"peaks":0,"flows":0,"maxStreak":1},)"
                R"("exceptionCount":{"outliers":0,"excludes":0,"overruns":0},)"
                R"("duration":{"total":805,"average":403,"max":405},)"
                R"("analysis":{"smoothValue":1105,"derivative":0,"smoothDerivative":0,"smoothAbsDerivative":0}})",
                ResultListener.getPayload(),
                "measurement ok");
        }

    private:
        static void registerMeasurement(int measurement, int duration) {
            MeasurementWriter.addMeasurement(measurement);
            const FlowMeterDriver expected(measurement);
            ResultWriter.addMeasurement(measurement, &expected);
            eventServer.publish(Topic::ProcessTime, duration);
        }
    };

    EventServer SchedulerTest::eventServer;
    TestEventClient SchedulerTest::MeasurementListener("measure", &eventServer);
    TestEventClient SchedulerTest::ResultListener("result", &eventServer);
    RingbufferPayload SchedulerTest::payload1, SchedulerTest::payload2;
    PayloadBuilder SchedulerTest::payloadBuilder;
    Serializer SchedulerTest::serializer(&payloadBuilder);
    DataQueue SchedulerTest::dataQueue(&SchedulerTest::eventServer, &serializer);

    MeasurementAggregator SchedulerTest::MeasurementWriter(&eventServer, &dataQueue, &payload1);
    ResultAggregator SchedulerTest::ResultWriter(&eventServer, &dataQueue, &payload2, MEASURE_INTERVAL_MICROS);
    Scheduler SchedulerTest::Scheduler(&eventServer, &dataQueue, &MeasurementWriter, &ResultWriter);
}
*/