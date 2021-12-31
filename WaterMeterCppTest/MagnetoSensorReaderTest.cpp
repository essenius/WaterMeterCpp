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

#include <regex>

#include "CppUnitTest.h"
#include "../WaterMeterCpp/MagnetoSensorReader.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// doesn't do much, just checking the interface works

namespace WaterMeterCppTest {
    TEST_CLASS(MagnetoSensorReaderTest) {
    public:
        TEST_METHOD(magnetoSensorReaderTest1) {
            MagnetoSensorReader reader;
            reader.begin();
            const auto result = reader.read();
            Assert::AreEqual(0, result.X);
            Assert::AreEqual(0, result.Y);
            Assert::AreEqual(0, result.Z);
        }
    };
}
