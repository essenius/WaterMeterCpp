// Copyright 2021-2022 Rik Essenius
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

#include <regex>

#include "CppUnitTest.h"
#include "TestEventClient.h"
#include "../WaterMeterCpp/Device.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
    TEST_CLASS(ChangePublisherTest) {
public:
    TEST_METHOD(changePublisherTest1) {
        long test = 1 << 24;
        Assert::AreEqual<long>(0x1000000, test, L"test");
        test += 8192;
        Assert::AreEqual<long>(0x1002000, test, L"test");
        test += 65536;
        Assert::AreEqual<long>(0x1012000, test, L"test");
    }
    };
}
