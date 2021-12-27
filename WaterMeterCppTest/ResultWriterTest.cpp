#include "pch.h"
#include "CppUnitTest.h"
#include "../WaterMeterCpp/ResultWriter.h"
#include "FlowMeterDriver.h"
#include <string.h>
#include <iostream>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
	TEST_CLASS(ResultWriterTest) {
	public:
		/*
		TEST_METHOD(ResultWriterIdleTest) {
			ResultWriter rw();
			Assert::AreEqual(6000L, rw.getFlushRate(), L"Default flush rate OK.");

			rw.setIdleFlushRate("10");
			rwd.setNonIdleFlushRate("5");
			Assert::AreEqual(10L, rwd.getFlushRate(), L"Updated flush rate from idle.");
			for (int i = 0; i < 10; i++) {
				FlowMeterDriver fmd(i, 0, 2400, 2402, false, false, false, 5 - i, false, false, 0);
				rwd.addMeasurement(2400 + i, 8000 + i, &fmd);
				if (i == 0) {
					Assert::AreEqual(10L, rwd.getFlushRate(), L"Flush rate stays idle when there is nothing special.");
				}
				Assert::AreEqual(i == 9, rwd.needsFlush());
                if (i == 9) {
                    rwd.flush();
					Assert::AreEqual("S,10,0,0,0,0,2400,2402,-4,0,0,0,8005,8009,0,815", rwd.getOutput(),
						L"10 idle points written after #10. LowPassFast and LPonHP get reported too.");
				}
			}			
		}

		TEST_METHOD(ResultWriterIdleOutlierTest) {
            ResultWriterDriver rwd;
            rwd.setIdleFlushRate(10);
            rwd.setNonIdleFlushRate(5);
			rwd.setCanFlush(true);
            for (int i = 0; i < 15; i++)
            {
                FlowMeterDriver fmd(5 + (i > 7 ? 200 : 0), 0, 2400 - i, 2385 + i, i > 7, false, false, i, i > 7, i > 9, 0);
                // Amplitude = 5 + (i > 7 ? 200 : 0), HighPass = 0, LowPassFast = 2400 - i, LowPassSlow = 2385 + i,
                // Exclude = i > 7, ExcludeAll = false, Flow = false, LowPassOnHighPass = i, Outlier = i > 7, Drift = i > 9

                rwd.addMeasurement(2400 + (i > 7 ? 200 : 0), i == 0 ? 2500 : 7993 + i, &fmd);
                Assert::AreEqual(i == 9 || i == 10 || i == 14, rwd.needsFlush(false), L"Needs flush");
                // #9 and #14 are on flush rate 10 because they were just reset and haven't seen any data pooints yet
                Assert::AreEqual((i < 8 || i == 9 || i == 14) ? 10L : 5L, rwd.getFlushRate(), L"FlushRate OK based on outliers");
                Assert::AreEqual(i == 9 || i == 10 || i == 14, rwd.needsFlush());
                // skip flush at i=9 so it kicks in at 10.
                if (i == 10 || i == 14) {
                    rwd.flush();
                }

                if (i < 9) {
                    Assert::AreEqual("S", rwd.getPrintBuffer(), L"printbuffer ok");
                }
                if (i <= 9) {
                    Assert::AreEqual(0u, strlen(rwd.getOutput()), L"Nothing written the first 9 points");
                } else if (i < 14) {
                    //"S,Measure,Flows,SumAmplitude,SumLPonHP,LowPassFast,LPonHP,Outliers,Waits,Excludes,AvgDelay,CRC"
                    Assert::AreEqual("S,10,0,0,0,0,2391,2394,9,2,0,2,7448,8002,0,45301", rwd.getOutput(),
                        "At 10 we get a summary with 2 outliers and 2 excludes");
                } else  {
                    Assert::AreEqual("S,5,0,0,0,0,2386,2399,14,5,5,5,8005,8007,0,63092", rwd.getOutput(),
                        "At 15 we get the next batch with 5 outliers and excludes");
                }
            }
		}

		TEST_METHOD(ResultWriterHeaderAndOverrunTest) {
			ResultWriterDriver rwd;
			rwd.setIdleFlushRate(1);
			rwd.setNonIdleFlushRate(1);
			rwd.setCanFlush(true);
			Assert::AreEqual("S,Measure,Flows,Peaks,SumAmplitude,SumLPonHP,LowPassFast,LowPassSlow,LPonHP,Outliers,Drifts,Excludes,AvgDuration,MaxDuration,Overruns,CRC",
				rwd.getHeader(), L"Header correctly written.");
			rwd.flush();

			FlowMeterDriver fmd(10, 0, 2400, 2401, false, false, false, 0, false, false, 0);
			rwd.addMeasurement(2398, 10125, &fmd);
			Assert::IsTrue(rwd.needsFlush(), L"Needs flush");
			rwd.flush();
			Assert::AreEqual("S,2398,0,0,0,0,2400,2401,0,0,0,0,10125,10125,1,36784", rwd.getOutput(), 
				L"1 point written with flush rate 1, measures contains sensor value, and overrun triggered.");
		}

		TEST_METHOD(ResultWriterFlowTest) {
			ResultWriterDriver rwd;
			rwd.setIdleFlushRate(10);
			rwd.setNonIdleFlushRate(5);
			rwd.setCanFlush(true);
			for (int i = 0; i < 10; i++) {
				FlowMeterDriver fmd(5 + (i > 7 ? 10 : 0), 0, 2400, 2398, false, false, i > 7, i, false, false, (i==5));
				rwd.addMeasurement(2400 + i, 8000 + i, &fmd);
				Assert::AreEqual(i == 9, rwd.needsFlush());
			}
			rwd.flush();
			Assert::AreEqual("S,10,2,1,30,17,2400,2398,9,0,0,0,8005,8009,0,21636", rwd.getOutput(),
				L"After 10 points, we get a summary with 2 flows, and SumAmplitude and SumLPonHP are populated");
		}

		TEST_METHOD(ResultWriterAllExcludedTest) {
			ResultWriterDriver rwd;
			rwd.setCanFlush(true);
			rwd.setIdleFlushRate(10);
			rwd.setNonIdleFlushRate(5);
			for (int i = 0; i < 15; i++) {
				FlowMeterDriver fmd(i == 0 ? 10000 : 2000, 0, 2400 -i, 2385 + i, i==2 || i==3, i==2, false, 10+i, i==2, false, 0);
				rwd.addMeasurement(2400 + (i > 7 ? 200 : 0), i == 0 ? 2500 : 7500 + i * 5, &fmd);
				Assert::AreEqual(i == 4 || i==14, rwd.needsFlush(), L"Needs flush");
				Assert::AreEqual(i < 2 || i >= 4 ? 10L : 5L, rwd.getFlushRate(), L"Flush rate is 10 the first two data points, and after the 5th. In between it is 5");
				if (i == 4) rwd.flush();
				if (i < 4) {
					Assert::AreEqual(0u, strlen(rwd.getOutput()), L"Nothing written before the 5th data point");
				}
				else if (i < 14) {
					Assert::AreEqual("S,5,0,0,0,0,2396,2389,14,1,0,4,6510,7520,0,30890", rwd.getOutput(), L"The right data written after 5 points");
				}
			}
			rwd.flush();
			Assert::AreEqual("S,10,0,0,0,0,2386,2399,24,0,0,0,7548,7570,0,37607", rwd.getOutput(), L"Without special events, the next flush is after 10 data points");
		} */
	};
}
