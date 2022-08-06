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
#include "TestEventClient.h"
#include "../WaterMeterCpp/ExtremeSearcher.h"

namespace WaterMeterCppTest {

    TEST(ExtremeSearcherTest, extremeSearcherMaxYTest) {
        ExtremeSearcher searcher(MaxY, {0, -1000}, nullptr);
        searcher.begin(5);
        EXPECT_EQ(-1000, searcher.extreme().y) << "Extreme Y value ok";
        EXPECT_EQ(0, searcher.extreme().x) << "Extreme X value ok";

        const float sampleY[] = {10, 20, 30, 35, 38, 35, 30, 20, 10};
        const float extremeY[] = {10, 20, 30, 35, 38, 38, 38, 38, 38};

        for (size_t i = 0; i < std::size(sampleY); i++) {
            constexpr int EXTREME_INDEX_FOUND = 6;
            searcher.addMeasurement({static_cast<float>(i), sampleY[i]});
            EXPECT_EQ(extremeY[i], searcher.extreme().y) << "Extreme value ok";
            EXPECT_EQ(i >= EXTREME_INDEX_FOUND, searcher.foundExtreme()) << "Extreme value ok";
        }
    }

    TEST(ExtremeSearcherTest, extremeSearcherMaxXTest) {
        ExtremeSearcher searcher(MaxX, {-1000, 0}, nullptr);
        searcher.begin(5);
        EXPECT_EQ(-1000, searcher.extreme().x) << "Extreme X value ok";
        EXPECT_EQ(0, searcher.extreme().y) << "Extreme Y value ok";

        constexpr float SAMPLE_X[] = {-10, -1, -3, 5, 12, 11, 13, 11, -4};
        constexpr float EXTREME_X[] = {-10, -1, -1, 5, 12, 12, 13, 13, 13};
        for (size_t i = 0; i < std::size(SAMPLE_X); i++) {
            constexpr int EXTREME_INDEX_FOUND = 8;
            searcher.addMeasurement({SAMPLE_X[i], static_cast<float>(i)});
            EXPECT_EQ(EXTREME_X[i], searcher.extreme().x) << "Extreme value ok";
            EXPECT_EQ(i >= EXTREME_INDEX_FOUND, searcher.foundExtreme()) << "Extreme value ok";
        }
    }

    TEST(ExtremeSearcherTest, extremeSearcherMinYTest) {
        ExtremeSearcher searcher(MinY, { 100, 1000 }, nullptr);
        searcher.begin(3);
        EXPECT_EQ(1000, searcher.extreme().y) << "Extreme Y value ok";
        EXPECT_EQ(100, searcher.extreme().x) << "Extreme X value ok";

        const float sampleY[] = { 10, 12, 8, 9, 6, 8, 1, 4 };
        const float extremeY[] = { 10, 10, 8, 8, 6, 6, 1, 1 };
        for (size_t i = 0; i < std::size(sampleY); i++) {
            constexpr int EXTREME_INDEX_FOUND = 7;
            searcher.addMeasurement({ static_cast<float>(i), sampleY[i] });
            EXPECT_EQ(extremeY[i], searcher.extreme().y) << "Extreme value ok";
            EXPECT_EQ(i >= EXTREME_INDEX_FOUND, searcher.foundExtreme()) << "Extreme value ok";
        }
    }

    TEST(ExtremeSearcherTest, extremeSearcherMinXTest) {
        ExtremeSearcher searcher(MinX, { 1000, 10 }, nullptr);
        searcher.begin(3);
        EXPECT_EQ(1000, searcher.extreme().x) << "Extreme X value ok";
        EXPECT_EQ(10, searcher.extreme().y) << "Extreme Y value ok";

        const float sampleX[] = { 100, 92, 91, 88, 75, 76, 77, 70, 71, 73 };
        const float extremeX[] = { 100, 92, 91, 88, 75, 75, 75, 70, 70, 70 };
        for (size_t i = 0; i < std::size(sampleX); i++) {
            constexpr int EXTREME_INDEX_FOUND = 9;
            searcher.addMeasurement({ sampleX[i], static_cast<float>(i) });
            EXPECT_EQ(extremeX[i], searcher.extreme().x) << "Extreme value ok";
            EXPECT_EQ(i >= EXTREME_INDEX_FOUND, searcher.foundExtreme()) << "Extreme value ok";
        }
    }

    TEST(ExtremeSearcherTest, extremeSearcherNoneTest) {
        ExtremeSearcher searcher(None, { 50, -50 }, nullptr);
        searcher.begin(3);
        EXPECT_EQ(50, searcher.extreme().x) << "Extreme X value ok";
        EXPECT_EQ(-50, searcher.extreme().y) << "Extreme Y value ok";

        constexpr float SAMPLE_X[] = { 100, 0, -100, 200 };
        constexpr float EXTREME_X[] = { 50, 50, 50, 50};
        for (size_t i = 0; i < std::size(SAMPLE_X); i++) {
            searcher.addMeasurement({ SAMPLE_X[i], static_cast<float>(i) });
            EXPECT_EQ(EXTREME_X[i], searcher.extreme().x) << "Extreme value not changed";
            EXPECT_EQ(false, searcher.foundExtreme()) << "No extreme";
        }
    }

    TEST(ExtremeSearcherTest, extremeSearcherNextTest) {
        ExtremeSearcher searcher1(MaxX, { -1000, 0 }, nullptr);
        ExtremeSearcher searcher2(MaxY, { 20, -2000 }, &searcher1);

        searcher2.begin(5);
        const auto nextSearcher = searcher2.next();
        EXPECT_EQ(-1000, nextSearcher->extreme().x) << "Extreme X value ok";
        EXPECT_EQ(0, nextSearcher->extreme().y) << "Extreme Y value ok";
        EXPECT_FALSE(nextSearcher->foundExtreme()) << "Extreme not found";
    }
}
