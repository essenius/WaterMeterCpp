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


#include "CppUnitTest.h"
#include "../WaterMeterCpp/Scheduler.h"
#include "../WaterMeterCpp/MeasurementWriter.h"
#include "../WaterMeterCpp/ResultWriter.h"
#include "../WaterMeterCpp/ArduinoMock.h"

#include "FlowMeterDriver.h"
#include "TestEventClient.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

constexpr int MEASURE_INTERVAL_MICROS = 10000;

namespace WaterMeterCppTest {


    TEST_CLASS(SchedulerTest) {
    public:
        static EventServer EventServer;
        static TestEventClient MeasurementListener;
        static TestEventClient ResultListener;
        static PayloadBuilder Builder1, Builder2;
        static MeasurementWriter MeasurementWriter;
        static ResultWriter ResultWriter;
        static Scheduler Scheduler;

        TEST_CLASS_INITIALIZE(schedulerClassInitalize) {
            EventServer.subscribe(&MeasurementListener, Topic::Measurement);
            EventServer.subscribe(&ResultListener, Topic::Result);
        }

        TEST_METHOD_INITIALIZE(schedulerMethodInitialize) {
            MeasurementWriter.begin();
            ResultWriter.begin();
            MeasurementListener.reset();
            ResultListener.reset();
        }

        TEST_METHOD(schedulerForcedDoubleWriteTest) {
            EventServer.publish(Topic::BatchSizeDesired, 1);
            EventServer.publish(Topic::IdleRate, 1);

            registerMeasurement(1000, 350);
            Scheduler.processOutput();
            Assert::AreEqual(1, MeasurementListener.getCallCount(), L"Measurement was sent");
            Assert::AreEqual(R"({"timestamp":"","measurements":[1000]})", MeasurementListener.getPayload(),
                             L"Payload OK");
            Assert::AreEqual(1, ResultListener.getCallCount(), L"Measurement was sent");
            Assert::AreEqual(
                R"({"timestamp":"","lastValue":1000,"summaryCount":{"samples":1,"peaks":0,"flows":0},)"
                R"("exceptionCount":{"outliers":0,"excludes":0,"overruns":0},)"
                R"("duration":{"total":350,"average":350,"max":350},)"
                R"("analysis":{"smoothValue":1000,"derivative":0,"smoothDerivative":0,"smoothAbsDerivative":0}})",
                ResultListener.getPayload(),
                L"Payload OK");
        }

        TEST_METHOD(schedulerAlternateWriteTest) {
            EventServer.publish(Topic::BatchSizeDesired, 2);
            Assert::AreEqual(2L, MeasurementWriter.getFlushRate(), L"measurement flush rate is 2");
            Assert::AreNotEqual(2L, ResultWriter.getFlushRate(), L"result flush rate is not 2");

            EventServer.publish(Topic::IdleRate, 2);
            Assert::AreEqual(2L, ResultWriter.getFlushRate(), L"result flush rate is 2");

            EventServer.publish(Topic::NonIdleRate, 33);
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
            Assert::AreEqual(R"({"timestamp":"","lastValue":1105,"summaryCount":{"samples":2,"peaks":0,"flows":0},)"
                             R"("exceptionCount":{"outliers":0,"excludes":0,"overruns":0},)"
                             R"("duration":{"total":805,"average":403,"max":405},)"
                             R"("analysis":{"smoothValue":1105,"derivative":0,"smoothDerivative":0,"smoothAbsDerivative":0}})",
                             ResultListener.getPayload(),
                             "measurement ok");


            /*
            SerialDriverMock serialDriver;
            serialDriver.begin();
            MeasurementWriterDriver measurementWriter;
            ResultWriterDriver resultWriter;
            BatchWriter infoWriter(1, 'I');
            SerialScheduler scheduler(&serialDriver, &measurementWriter, &resultWriter, &infoWriter, freeMemory);
            serialDriver.setNextReadResponse("A\n");
            Assert::IsTrue(scheduler.processInput(), L"processInput found inout 1");
            Assert::IsTrue(scheduler.isConnected(), L"Connected 1");
            // Connection command A is an exception - that immediately returns a startup message, and doesn't wait for processOutput.
            // This is because the headers don't adhere to the normal batch processing rules, and we aren't sending batches yet. 
            // Could be made a bit more intuitive, as this is the only place where processInput also sends output.
            Assert::AreEqual(
                "I,Version,0.4,27208\n"
                "M,M0,M1,M2,M3,M4,M5,M6,M7,M8,M9,CRC\n"
                "S,Measure,Flows,Peaks,SumAmplitude,SumLPonHP,LowPassFast,LowPassSlow,LPonHP,Outliers,Drifts,Excludes,AvgDuration,MaxDuration,Overruns,CRC\n", 
                serialDriver.getOutput(), "Arrive output OK");
            serialDriver.clearOutput();
    
            // acknowledge the header
            serialDriver.setNextReadResponse("Y\n");
            scheduler.processInput();
            Assert::IsTrue(scheduler.isConnected(), L"Connected 2");
    
            resultWriter.setIdleFlushRate(1);
            measurementWriter.setDesiredFlushRate(1);
            measurementWriter.addMeasurement(1000, 350);
            FlowMeterDriver expected1(0.0f, false, false, 0.0f, 0.0f, false, 1000.0f, 1000.0f, 0.0f, 0.0f, false, false, false, false, 0);
            resultWriter.addMeasurement(1000, 350, &expected1);
            scheduler.processOutput();
            Assert::AreEqual("M,1000,64644\nS,1000,0,0,0,0,1000,1000,0,0,0,0,350,350,0,15481\n", serialDriver.getOutput(), "Output OK after first batch");
            serialDriver.clearOutput();
    
            // acknowledge the data, case shouldn't matter
            serialDriver.setNextReadResponse("y\n");
            scheduler.processInput();
    
            serialDriver.setNextReadResponse("L 7\n");
            scheduler.processInput();
            Assert::AreEqual(7L, measurementWriter.getDesiredFlushRate(), L"measurement desired flush rate is 7");
            Assert::AreEqual(7L, measurementWriter.getFlushRate(), L"measurement flush rate is 7");
            Assert::AreNotEqual(7L, resultWriter.getIdleFlushRate(), L"idle rate is not 7");
            Assert::AreNotEqual(7L, resultWriter.getNonIdleFlushRate(), L"non-idle rate is not 7");
            Assert::AreNotEqual(7L, resultWriter.getFlushRate(), L"result flush rate is not 7");
    
            serialDriver.setNextReadResponse("I 11\n");
            scheduler.processInput();
            Assert::AreEqual(11L, resultWriter.getIdleFlushRate(), L"Idle flush rate is 11");
            Assert::AreEqual(7L, measurementWriter.getDesiredFlushRate(), L"measurement desired flush rate is still 7");
            Assert::AreNotEqual(11L, resultWriter.getNonIdleFlushRate(), L"non-idle rate is not 11");
    
            serialDriver.setNextReadResponse("N 13\n");
            scheduler.processInput();
            Assert::AreEqual(13L, resultWriter.getNonIdleFlushRate(), L"Non-idle flush rate is 13");
            Assert::AreEqual(7L, measurementWriter.getDesiredFlushRate(), L"measurement desired flush rate is still 7");
            Assert::AreEqual(11L, resultWriter.getIdleFlushRate(), L"Idle rate is still 11");
    
            // update idle flush rate
            serialDriver.setNextReadResponse("I 2\n");
            scheduler.processInput();
            Assert::AreEqual(2L, resultWriter.getIdleFlushRate(), L"Idle flush rate is 2");
    
            // update measurement log rate
            serialDriver.setNextReadResponse("L 2\n");
            scheduler.processInput();
            Assert::AreEqual(2L, measurementWriter.getDesiredFlushRate(), L"Desired log flush rate is 2");
    
            // uodate non-idle log rate
            serialDriver.setNextReadResponse("n 33\n");
            scheduler.processInput();
            Assert::AreEqual(33L, resultWriter.getNonIdleFlushRate(), L"Non-idle flush rate is 33");
    
            // get an overview of the rates. This follows the normal route via output.
            serialDriver.setNextReadResponse("R\n");
            scheduler.processInput();
    
            measurementWriter.addMeasurement(1100, 400);
            FlowMeterDriver expected2(0.0f, false, false, 0.0f, 0.0f, false, 1100.0f, 1100.0f, 0.0f, 0.0f, false, false, false, false, 0);
            resultWriter.addMeasurement(1000, 400, &expected2);
            scheduler.processOutput();
            Assert::AreEqual("I,Rates:current,log,2,result,2,desired,log,2,idle,2,non-idle,33,desired,2,23700\n", serialDriver.getOutput(), 
                "First round in second batch gives requested info as there was nothing else to print");
            serialDriver.clearOutput();
            // acknowledge the data
            serialDriver.setNextReadResponse("Y\n");
            scheduler.processInput();
    
            // request free memory
            serialDriver.setNextReadResponse("M\n");
            scheduler.processInput();
    
            measurementWriter.addMeasurement(1105, 405);
            FlowMeterDriver expected3(5.0f, false, false, 3.0f, 4.0f, false, 1101.0f, 1102.0f, 1.0f, 2.0f, false, false, false, false, 0);
            resultWriter.addMeasurement(1005, 405, &expected3);
            scheduler.processOutput();
            Assert::AreEqual("M,1100,1105,24603\nI,MemoryFree,3750,27390\n", serialDriver.getOutput(), 
                "Second round delivers measurement and suppresses output of result, but provides info (no other choice)");
            serialDriver.clearOutput();
            // acknowledge the data
            serialDriver.setNextReadResponse("Y\n");
            scheduler.processInput();
    
            measurementWriter.addMeasurement(1103, 410);
            FlowMeterDriver expected4(5.0f, false, false, 3.0f, 4.0f, false, 1101.0f, 1102.0f, 1.0f, 2.0f, false, false, false, false, 0);
            resultWriter.addMeasurement(1103, 410, &expected4);
            scheduler.processOutput();
            Assert::AreEqual("S,2,0,0,0,0,1102,1101,4,0,0,0,403,405,0,45329\n", serialDriver.getOutput(), "Third round delivers output of result");
            serialDriver.clearOutput();
            // disconnect
            serialDriver.setNextReadResponse("Q\n");
            scheduler.processInput();
            Assert::IsFalse(scheduler.isConnected(), L"Disconnect executed");
            Assert::IsFalse(measurementWriter.needsFlush());
    
            Assert::AreEqual(2L, measurementWriter.getFlushRate(), L"Flush rate is still 2 after disconnect");
    
            // now measurements should be reset, so this shouldn't trigger writes
            measurementWriter.addMeasurement(1107, 390);
            FlowMeterDriver expected5(6.0f, false, false, 3.0f, 4.0f, false, 1104.0f, 1106.0f, 2.0f, 3.0f, false, false, false, false, 0);
            resultWriter.addMeasurement(1107, 390, &expected5);
            Assert::IsFalse(scheduler.processOutput(), L"processOutput returns false (1)");
            Assert::IsFalse(scheduler.wasResultWritten(), L"Result not written");
            Assert::AreEqual("", serialDriver.getOutput(), "Disconnect causes silence");
    
            // this one neither
            measurementWriter.addMeasurement(1102, 401);
            FlowMeterDriver expected6(2.0f, false, false, 4.0f, 2.0f, false, 1103.0f, 1105.0f, 1.0f, 2.0f, false, false, false, false, 0);
            resultWriter.addMeasurement(1102, 401, &expected6);
            Assert::IsFalse(scheduler.processOutput(), L"processOutput returns false (2)");
            Assert::AreEqual("", serialDriver.getOutput(), "Second sample is silent too");
    
            // Switch on connection again
            serialDriver.setNextReadResponse("A\n");
            scheduler.processInput();
            Assert::IsTrue(scheduler.isConnected(), L"Connected 3");
            Assert::AreEqual(
                "I,Version,0.4,27208\n"
                "M,M0,M1,CRC\n"
                "S,Measure,Flows,Peaks,SumAmplitude,SumLPonHP,LowPassFast,LowPassSlow,LPonHP,Outliers,Drifts,Excludes,AvgDuration,MaxDuration,Overruns,CRC\n",
                serialDriver.getOutput(), "Arrive output OK");
            serialDriver.clearOutput();
    
            // acknowledge the data
            serialDriver.setNextReadResponse("Y\n");
            scheduler.processInput();
            Assert::IsTrue(scheduler.isConnected(), L"Connected 4");
    
            // Two more samples
            measurementWriter.addMeasurement(1104, 399);
            FlowMeterDriver expected7(4.0f, false, false, 2.0f, 3.0f, false, 1105.0f, 1103.0f, 5.0f, 4.0f, false, false, false, false, 0);
            resultWriter.addMeasurement(1104, 399, &expected7);
            Assert::IsTrue(scheduler.processOutput(), L"Output delivered after reconnect");
            Assert::IsTrue(scheduler.wasResultWritten(), L"Summmary result was written");
            Assert::AreEqual("S,4,0,0,0,0,1103,1105,3,0,0,0,400,410,0,30816\n", serialDriver.getOutput(),
                "Results gathering continuned during disconnected state, and includes the one from before the previous flush");
            serialDriver.clearOutput();
    
            // acknowledge the data
            serialDriver.setNextReadResponse("Y\n");
            scheduler.processInput();
    
            measurementWriter.addMeasurement(1101, 402);
            FlowMeterDriver expected8(1.0f, false, false, 3.0f, 0.0f, false, 1102.0f, 1104.0f, 2.0f, 4.0f, false, false, false, false, 0);
            resultWriter.addMeasurement(1101, 402, &expected8);
            Assert::IsTrue(scheduler.processOutput(), L"ProcessOutput did output");
            Assert::AreEqual("M,1104,1101,10011\n", serialDriver.getOutput(), "Second one after reconnect provides only the two measurements from after the reconnect");
        }
    
            TEST_METHOD(SerialSchedulerTimingTest) {
                SerialDriverMock serialDriver;
                serialDriver.begin();
                MeasurementWriterDriver measurementWriter;
                ResultWriterDriver resultWriter;
                BatchWriter infoWriter(1, 'I');
                SerialScheduler scheduler(&serialDriver, &measurementWriter, &resultWriter, &infoWriter, freeMemory);
                serialDriver.setNextReadResponse("A\n");
                Assert::IsTrue(scheduler.processInput(), L"ProcessInput finds input");
                Assert::IsTrue(scheduler.isConnected(), L"Connected 1");
                unsigned long prev = micros();
                shiftMicros(scheduler.RESPONSE_TIMEOUT_MICROS);
                unsigned long after = micros();
                Assert::IsFalse(scheduler.isConnected(), L"Disconnected due to lack of response");
                Assert::IsFalse(scheduler.processOutput(), L"Output not processed when disconnected");
    
                serialDriver.setNextReadResponse("A\n");
                scheduler.processInput();
                Assert::IsTrue(scheduler.isConnected(), L"Connected 2");
                serialDriver.setNextReadResponse("Y\n");
                scheduler.processInput();
                shiftMicros(0);
    
                // Ignore unknown commands
                serialDriver.setNextReadResponse("B\n");
                scheduler.processInput();
                Assert::IsFalse(serialDriver.inputAvailable(), L"No input available after ignoring unknown command");
    
                // This should not happen in practice - but making sure it doesn't break anything
                serialDriver.setNextReadResponse("B 345678901234567890123456789012345678901234567890123\n");
                Assert::IsTrue(serialDriver.inputAvailable(), L"Input is available if we put a really long input in that needs to be cut off ");
                Assert::AreEqual("B", serialDriver.getCommand(), L"Right command found in long command");
                Assert::AreEqual("34567890123456789012345678901234567890123456789" ,serialDriver.getParameter(), "Buffer overflow handled gracefully by cutting output");
    
            }
    
            TEST_METHOD(SerialSchedulerBatch20Test) {
                SerialDriverMock serialDriver;
                serialDriver.begin();
                MeasurementWriterDriver measurementWriter;
                ResultWriterDriver resultWriter;
                BatchWriter infoWriter(1, 'I');
                SerialScheduler scheduler(&serialDriver, &measurementWriter, &resultWriter, &infoWriter, freeMemory);
                serialDriver.setNextReadResponse("A\n");
                scheduler.processInput();
                Assert::IsTrue(scheduler.isConnected(), L"Connected 1");
                serialDriver.clearOutput();
                Assert::IsFalse(scheduler.processInput(), L"Without input, processInput returns false");
    
                serialDriver.setNextReadResponse("L 20\n");
                scheduler.processInput();
                serialDriver.setNextReadResponse("I 20\n");
                scheduler.processInput();
                serialDriver.setNextReadResponse("N 20\n");
                scheduler.processInput();
    
                Assert::AreEqual(20L, measurementWriter.getFlushRate(), L"Measurement flush rate OK");
                Assert::AreEqual(20L, resultWriter.getFlushRate());
    
                serialDriver.setNextReadResponse("Y\n");
                scheduler.processInput();
    
                for (int i = 0; i < 42; i++) {
                    measurementWriter.addMeasurement(1000 + i, 400 + i);
                    float f = (float)i;
                    FlowMeterDriver expected(f, false, false, f, f - 1, false, 990 + f, 992 + f, f - 10, f - 9, false, false, false, false, i % 10 == 0);
                    resultWriter.addMeasurement(1000 + i, 400 + i, &expected);
                    scheduler.processInput();
                    scheduler.processOutput();
                    switch (i) {
                    case 10: // Ask for rates in an intermediate cycle.
                        Assert::AreEqual("", serialDriver.getOutput(), L"No output expected for intermediate measurements");
                        serialDriver.setNextReadResponse("R\n");
                        break;
                    case 11: // The rates asked for in the previous cycle should be returned
                        Assert::AreEqual("I,Rates:current,log,20,result,20,desired,log,20,idle,20,non-idle,20,desired,20,62279\n", serialDriver.getOutput(), 
                            L"Rates expected the cycle after it's requested if there is nothing else to output");
                        Assert::IsFalse(scheduler.hasDelayedFlush());
                        serialDriver.clearOutput();
                        serialDriver.setNextReadResponse("Y\n");
                        break;
                    case 18: // Ask for a free memory report just before we expect a flush
                        Assert::AreEqual("", serialDriver.getOutput(), L"No output expected for intermediate measurements");
                        serialDriver.setNextReadResponse("M\n");
                        break;
                    case 19: // Three flushes are needed at the same time. First we expect the measurement
                        Assert::AreEqual("M,1000,1001,1002,1003,1004,1005,1006,1007,1008,1009,1010,1011,1012,1013,1014,1015,1016,1017,1018,1019,44140\n", serialDriver.getOutput());
                        Assert::IsTrue(scheduler.hasDelayedFlush());
                        serialDriver.clearOutput();
                        serialDriver.setNextReadResponse("Y\n");
                        break;
                    case 20: // .. next is the result summary
                        Assert::AreEqual("S,20,0,2,0,0,1011,1009,18,0,0,0,410,419,0,28946\n", serialDriver.getOutput());
                        Assert::IsTrue(scheduler.hasDelayedFlush());
                        serialDriver.clearOutput();
                        serialDriver.setNextReadResponse("Y\n");
                        break;
                    case 21: // .. and last the requested info
                        Assert::AreEqual("I,MemoryFree,3750,27390\n", serialDriver.getOutput());
                        Assert::IsFalse(scheduler.hasDelayedFlush());
                        serialDriver.clearOutput();
                        serialDriver.setNextReadResponse("Y\n");
                        break;
                    case 38:
                        Assert::AreEqual("", serialDriver.getOutput(), L"Break point");
                        break;
                    case 39: // Next flush, now we expect 2. First the measurements
                        Assert::AreEqual("M,1020,1021,1022,1023,1024,1025,1026,1027,1028,1029,1030,1031,1032,1033,1034,1035,1036,1037,1038,1039,58077\n", serialDriver.getOutput());
                        Assert::IsTrue(scheduler.hasDelayedFlush(), L"Delayed flush" );
                        serialDriver.clearOutput();
                        serialDriver.setNextReadResponse("Y\n");
                        break;
                    case 40: // .. and then the summary
                        Assert::AreEqual("S,20,0,2,0,0,1031,1029,38,0,0,0,430,439,0,62947\n", serialDriver.getOutput());
                        Assert::IsFalse(scheduler.hasDelayedFlush());
                        serialDriver.clearOutput();
                        serialDriver.setNextReadResponse("Y\n");
                        break;
                    default:
                        Assert::AreEqual("", serialDriver.getOutput(), L"No output expected for intermediate measurements");
                        break;
                    }
                }
            }
    
            TEST_METHOD(SerialSchedulerBatch200Test) {
                SerialDriverMock serialDriver;
                serialDriver.begin();
                MeasurementWriterDriver measurementWriter;
                ResultWriterDriver resultWriter;
                BatchWriter infoWriter(1, 'I');
                SerialScheduler scheduler(&serialDriver, &measurementWriter, &resultWriter, &infoWriter, freeMemory);
                serialDriver.setNextReadResponse("L 10\n");
                scheduler.processInput();
                serialDriver.setNextReadResponse("I 20\n");
                scheduler.processInput();
                serialDriver.setNextReadResponse("N 20\n");
                scheduler.processInput();
    
                serialDriver.setNextReadResponse("A\n");
                scheduler.processInput();
                Assert::IsTrue(scheduler.isConnected(), L"Connected 1");
                serialDriver.clearOutput();
                for (int i = 0; i < 2000; i++) {
                    measurementWriter.addMeasurement(1000 + i, 400 + i);
                    float f = (float)i;
                    FlowMeterDriver expected(f, false, false, f, f - 1, false, 990 + f, 992 + f, f - 10, f - 9, false, false, false, true, i % 10 == 0);
                    resultWriter.addMeasurement(1000 + i, 400 + i, &expected);
                    serialDriver.setNextReadResponse("Y\n");
                    scheduler.processInput();
                    if (scheduler.processOutput()) {
                        Logger::WriteMessage(serialDriver.getOutput());
                        serialDriver.clearOutput();
                    }
                }
            }
    
            TEST_METHOD(SerialSchedulerLog1Test) {
                SerialDriverMock serialDriver;
                serialDriver.begin();
                MeasurementWriterDriver measurementWriter;
                ResultWriterDriver resultWriter;
                BatchWriter infoWriter(1, 'I');
                SerialScheduler scheduler(&serialDriver, &measurementWriter, &resultWriter, &infoWriter, freeMemory);
    
                serialDriver.setNextReadResponse("I 1\n");
                scheduler.processInput();
                serialDriver.setNextReadResponse("L 20\n");
                scheduler.processInput();
                serialDriver.setNextReadResponse("N 20\n");
                scheduler.processInput();
    
                serialDriver.setNextReadResponse("A\n");
                scheduler.processInput();
    
                Assert::IsTrue(scheduler.isConnected(), L"Connected 1");
    
                FlowMeterDriver expected(1000.0, false, false, 1000.0, 999.0, false, 990.0, 992.0, 994., 10.0, false, false, false, false, 0);
                measurementWriter.addMeasurement(1000, 400);
                resultWriter.addMeasurement(1000, 400, &expected);
    
                Assert::AreEqual(1L, resultWriter.getFlushRate(), L"Flush rate for result ok");
    
                serialDriver.setNextReadResponse("I 10\n");
                scheduler.processInput();
    
                Assert::AreEqual(1L, resultWriter.getFlushRate(), L"Flush rate still 1 (we're in a cycle)");
    
                serialDriver.clearOutput();
                for (int i = 0; i < 40; i++) {
                    measurementWriter.addMeasurement(1000, 400);
                    resultWriter.addMeasurement(1000, 400, &expected);
                    serialDriver.setNextReadResponse("Y\n");
                    scheduler.processInput();
                    if (scheduler.processOutput()) {
                        Logger::WriteMessage(serialDriver.getOutput());
                        serialDriver.clearOutput();
                    }
                }
                Assert::AreEqual(10L, resultWriter.getDesiredFlushRate(), L"Desired flush rate for result OK");
                Assert::AreEqual(10L, resultWriter.getFlushRate(), L"Flush rate now OK"); */
        }

    private:
        static void registerMeasurement(int measurement, int duration) {
            MeasurementWriter.addMeasurement(measurement);
            FlowMeterDriver expected(measurement);
            ResultWriter.addMeasurement(measurement, &expected);
            EventServer.publish(Topic::ProcessTime, duration);
        }
    };

    EventServer SchedulerTest::EventServer;
    TestEventClient SchedulerTest::MeasurementListener("measure", &EventServer);
    TestEventClient SchedulerTest::ResultListener("result", &EventServer);
    PayloadBuilder SchedulerTest::Builder1, SchedulerTest::Builder2;
    MeasurementWriter SchedulerTest::MeasurementWriter(&EventServer, &Builder1);
    ResultWriter SchedulerTest::ResultWriter(&EventServer, &Builder2, MEASURE_INTERVAL_MICROS);
    Scheduler SchedulerTest::Scheduler(&EventServer, &MeasurementWriter, &ResultWriter);
}
