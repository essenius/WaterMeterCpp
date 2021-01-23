#include "pch.h"
#include "CppUnitTest.h"
#include "../WaterMeterCpp/BatchWriter.h"
#include "BatchWriterDriver.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
	TEST_CLASS(BatchWriterTest) {
	public:

		TEST_METHOD(BatchWriterCrcTest) {
			char buffer[] = "1234567890";
			unsigned int result = BatchWriter::getCrc(buffer);
			Assert::AreEqual(50554u, result);
		}

		TEST_METHOD(BatchWriterToStringTest) {
			BatchWriterDriver bw;
			Assert::AreEqual("0", bw.toStringDriver(0, 0));
			Assert::AreEqual("1", bw.toStringDriver(1, 5));
			Assert::AreEqual("3.1416", bw.toStringDriver(3.1416f, 4));

			Assert::AreEqual("-2700.85", bw.toStringDriver(-2700.847f, 2));
			Assert::AreEqual("-1903", bw.toStringDriver(-1903, 0));
			Assert::AreEqual("40007", bw.toStringDriver(40007u, 0));
			Assert::AreEqual("-1900", bw.toStringDriver(-1900.37554f, 0));
			Assert::AreEqual("1.01", bw.toStringDriver(1.010003f, 5));
		}

		TEST_METHOD(BatchWriterprintParamTest) {
			BatchWriterDriver bw;
			bw.newMessage();
			bw.writeParam(-1234.56789f);
			Assert::IsFalse(bw.needsFlush(), L"No flush needed");
			Assert::AreEqual("B,-1234.57", bw.getMessage());
		}

		TEST_METHOD(BatchWriterFlushTest)
		{
			BatchWriterDriver bw;
			bw.setDesiredFlushRate(3);
			bw.setCanFlush(true);
			Assert::AreEqual(3L, bw.getFlushRate(), L"can change flush rate directly after creation");
			bw.setDesiredFlushRate(2);
			Assert::AreEqual(2L, bw.getFlushRate(), L"can change flush rate before first message");
			Assert::IsTrue(bw.newMessage(), L"New message works with flush rate 2");
			bw.writeParam(1);
			bw.setDesiredFlushRate(3);
			Assert::AreEqual(2L, bw.getFlushRate(), L"flush rate not reset after message was added to batch");
			Assert::IsFalse(bw.needsFlush(), L"First write doesn't trigger flushing");
			Assert::AreEqual("", bw.getOutput(), "No output after first write of first batch");
			Assert::AreEqual("B,1", bw.getMessage(), "Buffer is OK after first write of first batch");
			Assert::IsTrue(bw.newMessage(), L"New message works after first message");
			bw.writeParam(2);
			Assert::IsTrue(bw.needsFlush(), L"Second write flushes");
			bw.flush();
			Assert::AreEqual("B,1,2,17852", bw.getOutput(), "Correct output was writte for first batch");
			Assert::AreEqual("B", bw.getMessage(), "Buffer was reset after first batch flush");
			Assert::IsTrue(bw.newMessage(), L"New message works after first batch flush");
			bw.writeParam(3);
			Assert::AreEqual(3L, bw.getFlushRate(), L"Flush rate updated after first flush");
			bw.setDesiredFlushRate(0);
			Assert::IsFalse(bw.needsFlush(), L"first of 3 messages not flushed");
			Assert::IsTrue(bw.newMessage(), L"New message works after first of 3 messages");
			bw.writeParam(4);
			Assert::IsFalse(bw.needsFlush(), L"second of 3 messages not flushed");
			Assert::IsTrue(bw.newMessage(), L"New message works after second of 3 messages");
			bw.writeParam(5);
			Assert::IsTrue(bw.needsFlush(), L"third write flushes");
			bw.flush();
			Assert::AreEqual("B", bw.getMessage(), "Buffer was reset after second batch flush");
			Assert::AreEqual(0L, bw.getFlushRate(), L"Flush rate was updated to 0 after second batch flush");
			Assert::AreEqual("B,3,4,5,9716", bw.getOutput(), "Correct output was written for second (3 message) batch");
			Assert::IsFalse(bw.newMessage(), L"newMessage returns false with flush rate 0");
			Assert::IsFalse(bw.needsFlush(), L"No flush needed when flush rate is 0");
		}

		TEST_METHOD(BatchWriterShutdownTest) {
			BatchWriterDriver bw;
			bw.setDesiredFlushRate(3);
			bw.setCanFlush(true);
			bw.newMessage();
			bw.writeParam(1);
			Assert::IsTrue(bw.needsFlush(true), L"Forcing flush preparation via shutdown parameter");
			Assert::AreEqual("B,1,125", bw.getMessage(), "Buffer was reset after second batch flush");
			bw.flush();
			Assert::AreEqual("B", bw.getMessage(), "buffer emptied");
			Assert::AreEqual("B,1,125", bw.getOutput(), "Correct output written for shutdown");
			Assert::IsFalse(bw.needsFlush(true), L"No flush needed");
		}

	};
}
