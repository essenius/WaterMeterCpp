// Copyright 2022 Rik Essenius
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
#include "../WaterMeterCpp/SafeCString.h"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
    TEST_CLASS(SafeCStringTest) {
public:
    TEST_METHOD(safeCStringSafeStrcpyTest) {
        char target[5];
        Assert::AreEqual("abc1", safeStrcpy(target, "abc123"), L"Target was cut to 4 characters");
        Assert::AreEqual("abc1", target, L"Target is same as function result");
        Assert::AreEqual("qwer", safeStrcpy(target, "qwer"), L"4  character source copied correctly");
        Assert::AreEqual("", safeStrcpy(target, ""), L"Empty string copied correctly");
        Assert::AreEqual("def", safeStrcpy(target, "def"), L"3 character source copied correctly");
        Assert::AreEqual("def", safeStrcpy(target, nullptr), L"Nothing changed when copying a nulltr");
    }

    TEST_METHOD(safeCStringSafeStrCatTest) {
        char target[4] = { 0 };
        Assert::AreEqual("ab", safeStrcat(target, "ab"), L"source concatenated to target");
        Assert::AreEqual("ab", target, L"target is same as function result");
        Assert::AreEqual("ab", safeStrcat(target, ""), L"Concatenating empty string doesn't change the target");
        Assert::AreEqual("ab", safeStrcat(target, nullptr), L"Concatenating nullptr doesn't change the target");
        Assert::AreEqual("abc", safeStrcat(target, "cde"), L"source cut off after max of target");
    }

    TEST_METHOD(safeCStringSafeSprintfTest) {
        char target[10] = { 0 };
        char testUpper[5] = { 0 };
        Assert::AreEqual(2, safeSprintf(target, "x%d", 7), L"Right number of characters written");
        Assert::AreEqual("x7", target, "right string written");

        Assert::AreEqual(12, safeSprintf(target, "'%s'", "1234567890"), L"Returns written bytes if we had enough space");
        Assert::AreEqual("'12345678", target, "right string written (not enough space)");
        Assert::AreEqual("", testUpper, "TestUpper not changed");
    }

    TEST_METHOD(safeCStringsafePointerSprintfTest) {
        char target[10] = { 0 };
        char* pointer = target;
        Assert::AreEqual(2, safePointerSprintf(pointer, target, "y%d", 4), L"Right expected number of characters written 1");
        Assert::AreEqual("y4", pointer, "right string written 1");
        pointer += 3;
        Assert::AreEqual(9, safePointerSprintf(pointer, target, "%s", "abcdefghi"), L"Right expected number of characters written 2");
        Assert::AreEqual("abcdef", pointer, "right string written 2 (less than expected");
        pointer += 10; 
        Assert::AreEqual(0, safePointerSprintf(pointer, target, "%s", "abcdefghi"), L"Nothing written as above upper bound");
        pointer -= 20;
        Assert::AreEqual(0, safePointerSprintf(pointer, target, "%s", "abcdefghi"), L"Nothing written as below lower bound");
        pointer = target;
        Assert::AreEqual("y4", pointer, L"Buffer untouched after wrong bounds 1");
        pointer += 3;
        Assert::AreEqual("abcdef", pointer, L"Buffer untouched after wrong bounds 2");
    }

    TEST_METHOD(safeCStringsafePointerStrcpyTest) {
        char target[7] = {};
        char* pointer = target;
        Assert::AreEqual("rxd", safePointerStrcpy(pointer, target, "rxd"), L"Right string returned 1");
        Assert::AreEqual("rxd", pointer, "right string in pointer");
        pointer += 4;
        Assert::AreEqual("94", safePointerStrcpy(pointer, target, "9410"), L"Right string returned 2 (cut)");
        Assert::AreEqual("94", pointer, "right string in pointer 2 (cut)");
        pointer += 10;
        char currentContent = *pointer;
        char* result =  safePointerStrcpy(pointer, target, "vwxyz");
        Assert::AreEqual<char>(currentContent, *result, L"Nothing written as above upper bound");
        pointer -= 30;
        currentContent = *pointer;
        result = safePointerStrcpy(pointer, target, "qrstu");
        Assert::AreEqual<char>(currentContent, *result, L"Nothing written as below lower bound");
        pointer = target;
        Assert::AreEqual("rxd", pointer, L"Buffer untouched after wrong bounds 1");
        Assert::AreEqual("rxd", safePointerStrcpy(pointer, target, nullptr), L"Buffer untouched if source is null");
        Assert::AreEqual("rxd", pointer, L"Pointer untouched after null source");
        pointer += 4;
        Assert::AreEqual("94", pointer, L"Buffer untouched after wrong bounds 2");
        Assert::AreEqual("", safePointerStrcpy(pointer, target, ""), L"Empty string returned");
        Assert::AreEqual("", pointer, L"Empty string copied");
    }

    };
}