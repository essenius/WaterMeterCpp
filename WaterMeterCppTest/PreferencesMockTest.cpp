// Copyright 2021-2022 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#include "pch.h"

#include "CppUnitTest.h"


#include <Preferences.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
    TEST_CLASS(PreferencesMockTest) {
public:
    TEST_METHOD(preferencesMockTest) {
        Preferences prefs;
        prefs.load();
        prefs.begin("test1");
        Assert::AreEqual(456u, prefs.getUInt("int2", 0), L"int works");
        Assert::AreEqual("valueX", prefs.getString("string2").c_str(), L"getString works");
        char buf[6];
        prefs.getBytes("bytes2", buf, sizeof buf);
        Assert::AreEqual(0, strncmp(buf, "abcdef", 6), L"getBytes OK");
        prefs.begin("test");
        prefs.putString("string1", "value1");
        prefs.putUInt("int1", 123);
        constexpr char BUFFER[6] { 0x31, 0x40, 0x25, 0x3a, 0x65, 0x7e };
        prefs.putBytes("bytes1", BUFFER, 6);
        prefs.end();
        prefs.begin("test", true);
        prefs.getBytes("bytes1", buf, sizeof buf);
        Assert::AreEqual(0, strncmp(buf, "1@%:e~", 6), L"getBytes OK 2");
        prefs.end();
        prefs.save();
        prefs.clear();
        prefs.load();
        Assert::IsFalse(prefs.isKey("bytes1"));
        Assert::IsTrue(prefs.begin("test"));
        Assert::IsTrue(prefs.isKey("bytes1"));
        prefs.end();
    }
    };
}