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

        constexpr FloatCoordinate SAMPLE[] = {
        {-9.7f, 2.2f}, {-9, 4.3f}, {-7.8f, 6.2f},{-6.2f, 7.8f},{-4.3f, 9},
        {-2.2f,9.7f},{0,10.0f},{2.2f,9.7f},{4.3f,9}, {6.2f, 7.8f} };
        constexpr float EXTREME_Y[] = {2.2f, 4.3f, 6.2f, 7.8f,  9, 9.7f, 10, 10, 10, 10};

        for (size_t i = 0; i < std::size(SAMPLE); i++) {
            constexpr int EXTREME_INDEX_FOUND = 9;
            searcher.addMeasurement(SAMPLE[i]);
            EXPECT_EQ(EXTREME_Y[i], searcher.extreme().y) << "Extreme value ok for i=" << i;
            EXPECT_EQ(i >= EXTREME_INDEX_FOUND, searcher.foundTarget()) << "Extreme value ok for i=" << i;
        }
    }

    TEST(ExtremeSearcherTest, extremeSearcherMaxXTest) {
        ExtremeSearcher searcher(MaxX, {-1000, 0}, nullptr);
        searcher.begin(5);
        EXPECT_EQ(-1000, searcher.extreme().x) << "Extreme X value ok";
        EXPECT_EQ(0, searcher.extreme().y) << "Extreme Y value ok";

        // arc with x=10*sin(i+3)*pi/14, y=10*cos(i+3)*pi/14, with a dent to force a local extreme
        constexpr FloatCoordinate SAMPLE[] = {
            {6.2f, 7.8f}, {7.8f, 6.2f},{9.3f,4.3f},{9.2f,2.2f},
            {10.0f,0.0f},{9.7f,-2.2f},{9.0f,-4.3f},{7.8f,-6.2f} };

        constexpr float EXTREME_X[] = {6.2f, 7.8f, 9.3f, 9.3f, 10.0f, 10.0f, 10.0f, 10.0f};
        for (size_t i = 0; i < std::size(SAMPLE); i++) {
            constexpr int EXTREME_INDEX_FOUND = 7;
            searcher.addMeasurement(SAMPLE[i]);
            EXPECT_EQ(EXTREME_X[i], searcher.extreme().x) << "Extreme value ok for i=" << i;
            EXPECT_EQ(i >= EXTREME_INDEX_FOUND, searcher.foundTarget()) << "Extreme foundfor i=" << i;
        }
    }

    TEST(ExtremeSearcherTest, extremeSearcherMinYTest) {
        ExtremeSearcher searcher(MinY, { 100, 1000 }, nullptr);
        searcher.begin(5);
        EXPECT_EQ(1000, searcher.extreme().y) << "Extreme Y value ok";
        EXPECT_EQ(100, searcher.extreme().x) << "Extreme X value ok";

        constexpr FloatCoordinate SAMPLE[] = {
            {7.8f, -7.9f}, {6.2f, -7.8f},{4.3f,-9.0f},{2.2f,-9.7f},
            {0.0f,-10.0f},{-2.2f,-9.7f},{-4.3f, -9.0f},{-6.2f,-7.8f} };
        constexpr float EXTREME_Y[] = { -7.9f, -7.9f, -9.0f, -9.7f, -10.0f, -10.0f, -10.0f, -10.0f };
        for (size_t i = 0; i < std::size(EXTREME_Y); i++) {
            constexpr int EXTREME_INDEX_FOUND = 7;
            searcher.addMeasurement(SAMPLE[i]);
            EXPECT_EQ(EXTREME_Y[i], searcher.extreme().y) << "Extreme value ok for i=" << i;
            EXPECT_EQ(i >= EXTREME_INDEX_FOUND, searcher.foundTarget()) << "Extreme found for i=" << i;
        }
    }

    TEST(ExtremeSearcherTest, extremeSearcherMinXTest) {
        ExtremeSearcher searcher(MinX, { 1000, 10 }, nullptr);
        searcher.begin(5);
        EXPECT_EQ(1000, searcher.extreme().x) << "Extreme X value ok";
        EXPECT_EQ(10, searcher.extreme().y) << "Extreme Y value ok";

        constexpr FloatCoordinate SAMPLE[] = {
            {-6.2f, -7.8f}, {-7.8f, -6.2f}, {-9.0f, -4.3f},{-6.0f,-2.2f},{-10.0f,0.0f},
            {-9.7f, 2.2f}, {-9.0f, 4.3f}, {-7.8f, 6.2f}, {-6.2f, 7.8f}, {-4.3f, 9.0f}, {-2.2f, 9.7f} };
        constexpr float EXTREME_X[] = { -6.2f, -7.8f, -9.0f, -9.0f, -10.0f, -10.0f, -10.0f, -10.0f, -10.0f, -10.0f, -10.0f };
        for (size_t i = 0; i < std::size(SAMPLE); i++) {
            constexpr int EXTREME_INDEX_FOUND = 7;
            searcher.addMeasurement(SAMPLE[i]);
            EXPECT_EQ(EXTREME_X[i], searcher.extreme().x) << "Extreme value ok for i=" << i;
            EXPECT_EQ(i >= EXTREME_INDEX_FOUND, searcher.foundTarget()) << "Extreme value ok for i=" << i;
        }
    }

    TEST(ExtremeSearcherTest, extremeSearcherNextTest) {
        ExtremeSearcher searcher1(MaxX, { -1000, 0 }, nullptr);
        ExtremeSearcher searcher2(MaxY, { 20, -2000 }, &searcher1);

        searcher2.begin(5);
        const auto nextSearcher = searcher2.next();
        EXPECT_EQ(-1000, nextSearcher->extreme().x) << "Extreme X value ok";
        EXPECT_EQ(0, nextSearcher->extreme().y) << "Extreme Y value ok";
        EXPECT_FALSE(nextSearcher->foundTarget()) << "Extreme not found";
    }
}
