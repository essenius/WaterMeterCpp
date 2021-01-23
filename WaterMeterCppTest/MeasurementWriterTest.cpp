#include "pch.h"
#include "CppUnitTest.h"
#include "../WaterMeterCpp/MeasurementWriter.h"
#include "MeasurementWriterDriver.h"
#include "BatchWriterDriver.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
	TEST_CLASS(MeasurementWriterTest) {
	public:


		TEST_METHOD(MeasurementWriterAddMeasurementTest) {
			remove("storage.txt");
			MeasurementWriterDriver mw;
			mw.setCanFlush(true);
			Assert::AreEqual(10L, mw.getFlushRate(), L"Default flush rate OK");
			mw.setDesiredFlushRate(2);
			Assert::AreEqual(2L, mw.getFlushRate(), L"Flush rate changed");
			Assert::AreEqual("M,M0,M1,CRC", mw.getHeader(), "output OK after header flush");
			mw.flush();
			mw.addMeasurement(1000, 2000);
			Assert::IsFalse(mw.needsFlush());
			Assert::AreEqual("M,1000", mw.getPrintBuffer(), "print buffer OK after first measurement");
			mw.addMeasurement(-1000, 3000);
			Assert::IsTrue(mw.needsFlush());
			mw.flush();
			Assert::AreEqual("M", mw.getPrintBuffer(), "print buffer OK after flush");
			Assert::AreEqual("M,1000,-1000,7679", mw.getOutput(), "output OK after flush");
		}
		TEST_METHOD(MeasurementWriterZeroFlushRateTest) {
			remove("storage.txt");
			MeasurementWriterDriver mw;
			mw.setCanFlush(true);
			Assert::AreEqual(10L, mw.getFlushRate(), L"Default flush rate OK");
			mw.setDesiredFlushRate(2L);
			mw.addMeasurement(1000, 2000);
			Assert::IsFalse(mw.needsFlush());
			mw.setDesiredFlushRate(0L);
			Assert::AreEqual(2L, mw.getFlushRate(), L"Flush rate not changed");
			mw.addMeasurement(3000, 4000);
			Assert::IsTrue(mw.needsFlush());
			Assert::AreEqual("M,1000,3000,55114", mw.getPrintBuffer(), "print buffer OK after first measurement");
			Assert::AreEqual(0L, mw.getFlushRate(), L"Default flush rate OK");
			mw.flush();
			mw.addMeasurement(5000, 6000);
			Assert::IsFalse(mw.needsFlush());
			Assert::AreEqual("M", mw.getPrintBuffer(), "print buffer OK after zero flush rate kicks in");
		}
	};
}
