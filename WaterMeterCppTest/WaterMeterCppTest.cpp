#include "pch.h"
#include "CppUnitTest.h"
#include <iostream>
#include "../WaterMeterCpp/BatchWriter.h"



using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest
{
	TEST_CLASS(WaterMeterCppTest)
	{
	public:
		
		TEST_METHOD(TestMethod1) 		{
			char test[] = "The quick brown fox";
			std::cout << BatchWriter::getCrc(test) << "\n";;
		}
	};
}
