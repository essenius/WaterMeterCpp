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
#include "SerialDriverMock.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;


namespace WaterMeterCppTest {

	TEST_CLASS(SerialDriverTest) {
	public:
		TEST_METHOD(SerialDriverTest1) {
			SerialDriverMock serialDriver;
			Assert::IsNull(serialDriver.getCommand(), L"Before begin, getCommand returns null");
			Assert::IsNull(serialDriver.getParameter(), L"Before begin, getParameter returns null");
			Assert::IsFalse(serialDriver.inputAvailable(), L"Before begin, inputAvailable returns false");

			serialDriver.begin();
			Assert::IsNull(serialDriver.getCommand(), L"After Begin but before input is available, getCommand returns null");

			serialDriver.setNextReadResponse("a\n");
			Assert::IsTrue(serialDriver.inputAvailable(), L"Input is available if we put a really long input in that needs to be cut off ");
			Assert::AreEqual("A", serialDriver.getCommand(), L"command A is accurately identified");
			Assert::IsNull(serialDriver.getParameter(), L"Command A has no parameter");
			serialDriver.clearInput();
			Assert::IsFalse(serialDriver.inputAvailable(), L"After handling the command, inputAvailable is false");

			serialDriver.setNextReadResponse("i 100\nq\n");
			Assert::IsTrue(serialDriver.inputAvailable(), L"Input is available if we put in two commands");
			Assert::AreEqual("I", serialDriver.getCommand(), L"command I is accurately identified");
			Assert::AreEqual("100", serialDriver.getParameter(), "parameter 100 is correctly identified");
			serialDriver.clearInput();
			Assert::IsTrue(serialDriver.inputAvailable(), L"Input is still available since another command is waiting");

			Assert::AreEqual("Q", serialDriver.getCommand(), L"command Q is accurately identified");
			Assert::IsNull(serialDriver.getParameter(), L"Command Q has no parameter");
			serialDriver.clearInput();
			Assert::IsFalse(serialDriver.inputAvailable(), L"No more commands after handling the second one");

			serialDriver.setNextReadResponse("B 345678901234567890123456789012345678901234567890123\n");
			Assert::IsTrue(serialDriver.inputAvailable(), L"Input is available if we put a really long input in that needs to be cut off ");
			Assert::AreEqual("B", serialDriver.getCommand(), L"Right command found in long command");
			Assert::AreEqual("34567890123456789012345678901234567890123456789", serialDriver.getParameter(), "Buffer overflow handled gracefully by cutting output");

		}
	};
}