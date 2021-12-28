#include "pch.h"
#include "CppUnitTest.h"
#include "../WaterMeterCpp/ResultWriter.h"
#include "FlowMeterDriver.h"
#include <string.h>
#include <iostream>

#include "TestEventClient.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

constexpr unsigned long MEASURE_INTERVAL_MICROS = 10UL * 1000UL;

namespace WaterMeterCppTest {
	TEST_CLASS(ResultWriterTest) {
	public:
		
		TEST_METHOD(resultWriterIdleTest) {
			EventServer eventServer;
			PayloadBuilder builder;
			TestEventClient rateListener("rateListener", &eventServer);
			eventServer.subscribe(&rateListener, Topic::Rate);

			ResultWriter writer(&eventServer, &builder, MEASURE_INTERVAL_MICROS);
			writer.begin();

			Assert::AreEqual(1, rateListener.getCallCount(), L"rate set");
			Assert::AreEqual(6000L, writer.getFlushRate(), L"Default flush rate OK.");
			Assert::AreEqual("6000", rateListener.getPayload(), L"rate was published");

			eventServer.publish(Topic::IdleRate, 10);
			Assert::AreEqual(2, rateListener.getCallCount(), L"rate set again");
			Assert::AreEqual("10", rateListener.getPayload(), L"rate was taken from idlerate");
			Assert::AreEqual(10L, writer.getFlushRate(), L"Default flush rate OK.");

			eventServer.publish(Topic::NonIdleRate, 5);
			Assert::AreEqual(2, rateListener.getCallCount(), L"no new rate from non-idle");
			Assert::AreEqual(10L, writer.getFlushRate(), L"Flush rate not changed from non-idle.");

			for (int i = 0; i < 10; i++) {
				FlowMeterDriver fmd(2400 + i, 5, 7);
				writer.addMeasurement(2400 + i, &fmd);
				eventServer.publish(Topic::ProcessTime, 1000 + i * 2);
				if (i == 0) {
					Assert::AreEqual(10L, writer.getFlushRate(), L"Flush rate stays idle when there is nothing special.");
				}
				Assert::AreEqual(i == 9, writer.needsFlush());
                if (i == 9) {
					Assert::AreEqual(
						R"({"timestamp":"","lastValue":2409,"summaryCount":{"samples":10,"peaks":0,"flows":0},)"
						R"("exceptionCount":{"outliers":0,"excludes":0,"overruns":0},)"
						R"("duration":{"total":10090,"average":1009,"max":1018},)"
						R"("analysis":{"smoothValue":2409,"derivative":5,"smoothDerivative":7,"smoothAbsDerivative":0}})",
						writer.getMessage(), 
						L"10 idle points prepared after #10. ");
					writer.flush();

				}
			}			
		}

		TEST_METHOD(resultWriterIdleOutlierTest) {
			EventServer eventServer;
			PayloadBuilder builder;
			TestEventClient rateListener("rateListener", &eventServer);
			eventServer.subscribe(&rateListener, Topic::Rate);

			ResultWriter writer(&eventServer, &builder, MEASURE_INTERVAL_MICROS);
			writer.begin();
			eventServer.publish(Topic::IdleRate, "10");
			eventServer.publish(Topic::NonIdleRate, "5");
			Assert::AreEqual(2, rateListener.getCallCount(), L"two rate announcements");
			Assert::AreEqual(10L, writer.getFlushRate(), L"Flush rate at last set idle rate.");

			for (int i = 0; i < 15; i++) {
				int measurement = 2400 + (i > 7 ? 200 : 0);
				FlowMeterDriver fmd(measurement, -3, 1, false, false, i > 7, i > 7);
				writer.addMeasurement(measurement, &fmd);
				eventServer.publish(Topic::ProcessTime, 7993 + i);
				Assert::AreEqual(i == 9 || i == 10 || i == 14, writer.needsFlush(false), (L"Needs flush - " + std::to_wstring(i)).c_str());
	
			    // #9 and #14 are on flush rate 10 because they were just reset and haven't seen any data points yet
				Assert::AreEqual((i < 8 || i == 9 || i == 14) ? 10L : 5L, writer.getFlushRate(), L"FlushRate OK based on outliers");
				Assert::AreEqual(i == 9 || i == 10 || i == 14, writer.needsFlush(), L"9, 10 and 14 need flush");
				// skip flush at i=9 so it kicks in at 10.

                if (i == 9 || i == 10) {
					Assert::AreEqual(
					    R"({"timestamp":"","lastValue":2600,"summaryCount":{"samples":10,"peaks":0,"flows":0},)"
						        R"("exceptionCount":{"outliers":2,"excludes":2,"overruns":0},)"
						        R"("duration":{"total":79975,"average":7998,"max":8002},)"
						        R"("analysis":{"smoothValue":2600,"derivative":-3,"smoothDerivative":1,"smoothAbsDerivative":0}})",
						writer.getMessage(),
						"At 10 we get a summary with 2 outliers and 2 excludes");
				} else if (i == 14) {
					Assert::AreEqual(
					    R"({"timestamp":"","lastValue":2600,"summaryCount":{"samples":5,"peaks":0,"flows":0},)"
				    		R"("exceptionCount":{"outliers":5,"excludes":5,"overruns":0},)"
					    	R"("duration":{"total":40025,"average":8005,"max":8007},)"
						    R"("analysis":{"smoothValue":2600,"derivative":-3,"smoothDerivative":1,"smoothAbsDerivative":0}})",
						writer.getMessage(), 
						"At 15 we get the next batch with 5 outliers and excludes");
				} else {
					Assert::AreEqual(
						R"({"timestamp":"")", 
						writer.getMessage(), 
						(L"Empty buffer at " + std::to_wstring(i)).c_str());
				}
				if (i == 10) {
					writer.flush();
				}
			}
		}

		TEST_METHOD(resultWriterOverrunTest) {
			EventServer eventServer;
			PayloadBuilder builder;
			ResultWriter writer(&eventServer, &builder, MEASURE_INTERVAL_MICROS);
			writer.begin();
			eventServer.publish(Topic::IdleRate, 1);
			eventServer.publish(Topic::NonIdleRate, 1);
			FlowMeterDriver fmd(2400, 0, 1);
			writer.addMeasurement(2398, &fmd);
			eventServer.publish(Topic::ProcessTime, 10125L);
			Assert::IsTrue(writer.needsFlush(), L"Needs flush");
			Assert::AreEqual(
				R"({"timestamp":"","lastValue":2398,"summaryCount":{"samples":1,"peaks":0,"flows":0},)"
				R"("exceptionCount":{"outliers":0,"excludes":0,"overruns":1},)"
				R"("duration":{"total":10125,"average":10125,"max":10125},)"
				R"("analysis":{"smoothValue":2400,"derivative":0,"smoothDerivative":1,"smoothAbsDerivative":0}})", 
				writer.getMessage(), 
				L"1 point written, flush rate 1, measures contains sensor value, overrun triggered.");
		}

		TEST_METHOD(resultWriterFlowTest) {
			EventServer eventServer;
			PayloadBuilder builder;
			ResultWriter writer(&eventServer, &builder, MEASURE_INTERVAL_MICROS);
			writer.begin();
			eventServer.publish(Topic::IdleRate, 10);
			eventServer.publish(Topic::NonIdleRate, 5);
			for (int i = 0; i < 10; i++) {
				int measurement = 2400 + (i % 2) * 50;
				FlowMeterDriver fmd(measurement, (i % 2) * 50, 1, i > 7, i == 5);

				writer.addMeasurement(measurement, &fmd);
				eventServer.publish(Topic::ProcessTime, 8000 + i);
				Assert::AreEqual(i == 9, writer.needsFlush());
			}
			Assert::AreEqual(
				R"({"timestamp":"","lastValue":2450,"summaryCount":{"samples":10,"peaks":1,"flows":2},)"
				R"("exceptionCount":{"outliers":0,"excludes":0,"overruns":0},)"
				R"("duration":{"total":80045,"average":8005,"max":8009},)"
				R"("analysis":{"smoothValue":2450,"derivative":50,"smoothDerivative":1,"smoothAbsDerivative":0}})", 
				writer.getMessage(),
				L"After 10 points, we get a summary with 2 flows");
		}

		TEST_METHOD(resultWriterAllExcludedTest) {
			EventServer eventServer;
			PayloadBuilder builder;
			ResultWriter writer(&eventServer, &builder, MEASURE_INTERVAL_MICROS);
			writer.begin();
			eventServer.publish(Topic::IdleRate, 10);
			eventServer.publish(Topic::NonIdleRate, 5);
			
			for (int i = 0; i < 15; i++) {
				FlowMeterDriver fmd(2400 + (i==2)*10, 0, 0, false, false,
					i == 2, i == 2 || i == 3, i == 2);
				writer.addMeasurement((i == 0) * 7600 + 2400 + (i > 7 ? 200 : 0), &fmd);
			    eventServer.publish( Topic::ProcessTime, i == 0 ? 2500 : 7500 + i * 5);
				Assert::AreEqual(i == 4 || i == 14, writer.needsFlush(), L"Needs flush");
				Assert::AreEqual(i < 2 || i >= 4 ? 10L : 5L, writer.getFlushRate(), L"Flush rate is 10 the first two data points, and after the 5th. In between it is 5");
				if (i != 4 && i != 14) {
					Assert::AreEqual(R"({"timestamp":"")", writer.getMessage(), L"Only data prepared in rounds 5 and 15");
				}
				else if (i == 4) {
					Assert::AreEqual(
						R"({"timestamp":"","lastValue":2400,"summaryCount":{"samples":5,"peaks":0,"flows":0},)"
						R"("exceptionCount":{"outliers":1,"excludes":4,"overruns":0},)"
                        R"("duration":{"total":32550,"average":6510,"max":7520},)"
						R"("analysis":{"smoothValue":2400,"derivative":0,"smoothDerivative":0,"smoothAbsDerivative":0}})", 
						writer.getMessage(), 
						L"The right data prepared after 5 points");
					writer.flush();
				}
			}
			Assert::AreEqual(
				R"({"timestamp":"","lastValue":2600,"summaryCount":{"samples":10,"peaks":0,"flows":0},)"
				R"("exceptionCount":{"outliers":0,"excludes":0,"overruns":0},)"
				R"("duration":{"total":75475,"average":7548,"max":7570},)"
				R"("analysis":{"smoothValue":2400,"derivative":0,"smoothDerivative":0,"smoothAbsDerivative":0}})", 
				writer.getMessage(),
				L"Without special events, the next flush is after 10 data points");
		}

		TEST_METHOD(resultWriterDisconnectTest) {
			EventServer eventServer;
			PayloadBuilder builder;
			ResultWriter writer(&eventServer, &builder, MEASURE_INTERVAL_MICROS);
			writer.begin();
			eventServer.publish(Topic::IdleRate, 5);
			eventServer.publish(Topic::NonIdleRate, 5);
			FlowMeterDriver fmd(500);
			for (int i = 0; i < 3; i++) {
				writer.addMeasurement(500, &fmd);
				eventServer.publish(Topic::ProcessTime, 2500 + 10 * i);
				Assert::IsFalse(writer.needsFlush(), L"First 3 measurements don't need flush");
			}
			eventServer.publish(Topic::Disconnected, "");
			for (int i = 0; i < 4; i++) {
				writer.addMeasurement(500, &fmd);
				eventServer.publish(Topic::ProcessTime, 2500 - 10 * i);
				Assert::IsFalse(writer.needsFlush(), L"Next 4 measurements still don't need flush (can't after 5th)");
			}

			eventServer.publish(Topic::Connected, 1);
			for (int i = 0; i < 2; i++) {
				writer.addMeasurement(500, &fmd);
				eventServer.publish(Topic::ProcessTime, 2510 - 10 * i);
				Assert::IsFalse(writer.needsFlush(), L"Next 2 measurements still don't flush (as waiting for next round");
			}
			writer.addMeasurement(500, &fmd);
			eventServer.publish(Topic::ProcessTime, 2500);
		    Assert::IsTrue(writer.needsFlush(), L"next round, so needsFlush returns true");
			Assert::AreEqual(
				R"({"timestamp":"","lastValue":500,"summaryCount":{"samples":10,"peaks":0,"flows":0},)"
				R"("exceptionCount":{"outliers":0,"excludes":0,"overruns":0},)"
				R"("duration":{"total":24980,"average":2498,"max":2520},)"
				R"("analysis":{"smoothValue":500,"derivative":0,"smoothDerivative":0,"smoothAbsDerivative":0}})",
				writer.getMessage(), 
				"10 samples (multiple of 5)");
		}

	};
}
