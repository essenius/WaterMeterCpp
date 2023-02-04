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

#include "gtest/gtest.h"
#include "../WaterMeterCpp/PayloadBuilder.h"

namespace WaterMeterCppTest {
    
    TEST(PayloadBuilderTest, payloadBuilderAlmostFullTest) {
        PayloadBuilder builder;
        builder.initialize();
        // fill with just under the limit of 492 characters
        for (int i = 0; i < 49; i++) {
            builder.writeParam("x", 12345);
        }
        EXPECT_FALSE(builder.isAlmostFull()) << "Not almost full";

        // take it across the limit
        builder.writeParam("x", "1234");
        EXPECT_TRUE(builder.isAlmostFull()) << "Almost full";
    }

    TEST(PayloadBuilderTest, payloadBuilderParamTest) {
        PayloadBuilder builder;
        builder.initialize();
        builder.writeArrayStart("array");
        builder.writeArrayEnd();
        builder.writeGroupStart("testdata");
        builder.writeParam("pi", 3.1415926535);
        builder.writeParam("float", 1.00);
        builder.writeParam("int", 1);
        builder.writeParam("long", 2L);
        builder.writeParam("unsigned long", 3UL);
        builder.writeGroupEnd();
        builder.writeGroupEnd();
        EXPECT_STREQ(R"({"array":[],"testdata":{"pi":3.14,"float":1,"int":1,"long":2,"unsigned long":3}})", builder.toString()) << "Content OK";
    }
}
