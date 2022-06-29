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

#include "gtest/gtest.h"
#include "../WaterMeterCpp/SafeCString.h"

namespace WaterMeterCppTest {
    
    TEST(SafeCStringTest, safeCStringSafeStrcpyTest) {
        char target[5];
        EXPECT_STREQ("abc1", safeStrcpy(target, "abc123")) << "Target was cut to 4 characters";
        EXPECT_STREQ("abc1", target) << "Target is same as function result";
        EXPECT_STREQ("qwer", safeStrcpy(target, "qwer")) << "4  character source copied correctly";
        EXPECT_STREQ("", safeStrcpy(target, "")) << "Empty string copied correctly";
        EXPECT_STREQ("def", safeStrcpy(target, "def")) << "3 character source copied correctly";
        EXPECT_STREQ("def", safeStrcpy(target, nullptr)) << "Nothing changed when copying a nulltr";
    }

    TEST(SafeCStringTest, safeCStringSafeStrCatTest) {
        char target[4] = {0};
        EXPECT_STREQ("ab", safeStrcat(target, "ab")) << "source concatenated to target";
        EXPECT_STREQ("ab", target) << "target is same as function result";
        EXPECT_STREQ("ab", safeStrcat(target, "")) << "Concatenating empty string doesn't change the target";
        EXPECT_STREQ("ab", safeStrcat(target, nullptr)) << "Concatenating nullptr doesn't change the target";
        EXPECT_STREQ("abc", safeStrcat(target, "cde")) << "source cut off after max of target";
    }

    TEST(SafeCStringTest, safeCStringSafeSprintfTest) {
        char target[10] = {0};
        constexpr char TEST_UPPER[5] = {0};
        EXPECT_EQ(2, safeSprintf(target, "x%d", 7)) << "Right number of characters written";
        EXPECT_STREQ("x7", target) << "right string written";
        EXPECT_EQ(12, safeSprintf(target, "'%s'", "1234567890")) << "Returns written bytes if we had enough space";
        EXPECT_STREQ("'12345678", target) << "right string written (not enough space)";
        EXPECT_STREQ("", TEST_UPPER) << "TestUpper not changed";
    }

    TEST(SafeCStringTest, safeCStringsafePointerSprintfTest) {
        char target[10] = {0};
        char* pointer = target;
        EXPECT_EQ(2, safePointerSprintf(pointer, target, "y%d", 4)) << "Right expected number of characters written 1";
        EXPECT_STREQ("y4", pointer) << "right string written 1";
        pointer += 3;
        EXPECT_EQ(9, safePointerSprintf(pointer, target, "%s", "abcdefghi")) << "Right expected number of characters written 2";
        EXPECT_STREQ("abcdef", pointer) << "right string written 2 (less than expected";
        pointer += 10;
        EXPECT_EQ(0, safePointerSprintf(pointer, target, "%s", "abcdefghi")) << "Nothing written as above upper bound";
        pointer -= 20;
        EXPECT_EQ(0, safePointerSprintf(pointer, target, "%s", "abcdefghi")) << "Nothing written as below lower bound";
        pointer = target;
        EXPECT_STREQ("y4", pointer) << "Buffer untouched after wrong bounds 1";
        pointer += 3;
        EXPECT_STREQ("abcdef", pointer) << "Buffer untouched after wrong bounds 2";
    }

    TEST(SafeCStringTest, safeCStringsafePointerStrcpyTest) {
        char target[7] = {};
        char* pointer = target;
        EXPECT_STREQ("rxd", safePointerStrcpy(pointer, target, "rxd")) << "Right string returned 1";
        EXPECT_STREQ("rxd", pointer) << "right string in pointer";
        pointer += 4;
        EXPECT_STREQ("94", safePointerStrcpy(pointer, target, "9410")) << "Right string returned 2 (cut)";
        EXPECT_STREQ("94", pointer) << "right string in pointer 2 (cut)";
        pointer += 10;
        char currentContent = *pointer;
        const char* result = safePointerStrcpy(pointer, target, "vwxyz");
        EXPECT_EQ(currentContent, *result) << "Nothing written as above upper bound";
        pointer -= 30;
        currentContent = *pointer;
        result = safePointerStrcpy(pointer, target, "qrstu");
        EXPECT_EQ(currentContent, *result) << "Nothing written as below lower bound";
        pointer = target;
        EXPECT_STREQ("rxd", pointer) << "Buffer untouched after wrong bounds 1";
        EXPECT_STREQ("rxd", safePointerStrcpy(pointer, target, nullptr)) << "Buffer untouched if source is null";
        EXPECT_STREQ("rxd", pointer) << "Pointer untouched after null source";
        pointer += 4;
        EXPECT_STREQ("94", pointer) << "Buffer untouched after wrong bounds 2";
        EXPECT_STREQ("", safePointerStrcpy(pointer, target, "")) << "Empty string returned";
        EXPECT_STREQ("", pointer) << "Empty string copied";
    }
}
