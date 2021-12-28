#include "pch.h"

#include "CppUnitTest.h"
#include "TestEventClient.h"
#include "../WaterMeterCpp/MeasurementWriter.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
	TEST_CLASS(MeasurementWriterTest) {
	public:


		TEST_METHOD(measurementWriterAddMeasurementTest) {
			EventServer eventServer;
			PayloadBuilder builder;
			// not using timestamps to make the testing easier. We've tested them elsewhere
			MeasurementWriter writer(&eventServer, &builder);
			writer.begin();
			Assert::AreEqual(50L, writer.getFlushRate(), L"Default flush rate OK");
			eventServer.publish(Topic::BatchSizeDesired, "5");
			Assert::AreEqual(5L, writer.getFlushRate(), L"Flush rate changed");
			eventServer.publish(Topic::BatchSizeDesired, "DEFAULT");
			Assert::AreEqual(50L, writer.getFlushRate(), L"Flush rate changed back to default");
			eventServer.publish(Topic::BatchSizeDesired, 2);
			Assert::AreEqual(2L, writer.getFlushRate(), L"Flush rate changed");
			writer.flush();
			writer.addMeasurement(1000);
			Assert::IsFalse(writer.needsFlush());
			Assert::AreEqual(R"({"timestamp":"","measurements":[1000)", writer.getMessage(), "buffer OK after first measurement");
			writer.addMeasurement(-1000);
			Assert::IsTrue(writer.needsFlush(), L"Needs flush after two measurements");
			Assert::AreEqual(R"({"timestamp":"","measurements":[1000,-1000]})", writer.getMessage(), "buffer OK after second measurement");
			writer.flush();
			Assert::AreEqual(R"({"timestamp":"","measurements":[)", writer.getMessage(), "buffer OK after flush");

		}

		TEST_METHOD(measurementWriterZeroFlushRateTest) {
			EventServer eventServer;
			PayloadBuilder builder;
			TestEventClient batchSizeListener("bsl", &eventServer);
			eventServer.subscribe(&batchSizeListener, Topic::BatchSize);
			MeasurementWriter writer(&eventServer, &builder);
			writer.begin();
			Assert::AreEqual(1, batchSizeListener.getCallCount(), L"batch size set");
			Assert::AreEqual("50", batchSizeListener.getPayload(), L"batch size is 50");
			Assert::AreEqual(50L, writer.getFlushRate(), L"Default flush rate OK");

		    batchSizeListener.reset();
			eventServer.publish(Topic::BatchSizeDesired, 2L);
			Assert::AreEqual(1, batchSizeListener.getCallCount(), L"batch size changed (no measurements yet)");
			Assert::AreEqual("2", batchSizeListener.getPayload(), L"batch size is 2");

			batchSizeListener.reset();
			writer.addMeasurement(1000);
			Assert::IsFalse(writer.needsFlush());
			// -1 should clip to 0;
			eventServer.publish(Topic::BatchSizeDesired, -1L);
			Assert::AreEqual(0, batchSizeListener.getCallCount(), L"batch size not changed");
			Assert::AreEqual(2L, writer.getFlushRate(), L"Flush rate not changed");
			writer.addMeasurement(3000);
			Assert::IsTrue(writer.needsFlush());
			Assert::AreEqual(R"({"timestamp":"","measurements":[1000,3000]})", writer.getMessage(), "buffer OK after two measurements");
			Assert::AreEqual(0L, writer.getFlushRate(), L"Flush rate changed");
			writer.flush();
			writer.addMeasurement(5000);
			Assert::IsFalse(writer.needsFlush());
			Assert::AreEqual(R"({"timestamp":"","measurements":[)", writer.getMessage(), "buffer OK after zero flush rate kicks in");

			// check whether failure to write is handled OK
			eventServer.publish(Topic::BatchSizeDesired, 2L);
			Assert::AreEqual(2L, writer.getFlushRate(), L"Flush rate changed back to 2");
			writer.addMeasurement(-2000);
			eventServer.publish(Topic::Disconnected, 1);
			writer.addMeasurement(-3000);
			Assert::AreEqual(R"({"timestamp":"","measurements":[)", writer.getMessage(), "buffer flushed since we can't write");


		}
	};
}
