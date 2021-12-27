#include "pch.h"
#include "CppUnitTest.h"
#include "../WaterMeterCpp/PayloadBuilder.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
	TEST_CLASS(PayloadBuilderTest) {
public:

	TEST_METHOD(PayloadBuilderParamTest) {
		PayloadBuilder builder;
		builder.initialize();
		builder.writeArrayStart("array");
		builder.writeArrayEnd();
		builder.writeGroupStart("testdata");
		builder.writeParam("pi", 3.1415926535f);
		builder.writeParam("float", 1.00f);
		builder.writeParam("int", 1);
		builder.writeParam("long", 2L);
		builder.writeParam("unsigned long", 3UL);
		builder.writeGroupEnd();
		builder.writeGroupEnd();
		Assert::AreEqual("{\"array\":[],\"testdata\":{\"pi\":3.14,\"float\":1,\"int\":1,\"long\":2,\"unsigned long\":3}}", builder.toString());
	}

	TEST_METHOD(PayloadBuilderAlmostFullTest) {
		PayloadBuilder builder;
		builder.initialize();
		// fill with just under the limit of 492 characters
		for (int i=0; i<49; i++) {
			builder.writeParam("x", 12345);
		}
		Assert::IsFalse(builder.isAlmostFull());
		// take it across the limit
		builder.writeParam("x", "1234");
		Assert::IsTrue(builder.isAlmostFull());
	}
	};
}