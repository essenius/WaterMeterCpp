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
#include "../WaterMeterCpp/WiFiClientFactory.h"

namespace WaterMeterCppTest {
    using WaterMeter::TlsConfig;
    using WaterMeter::WiFiClientFactory;

    TEST(WifiClientFactoryTest, wifiClientFactoryInsecureClientTest) {
        constexpr TlsConfig Config{nullptr, nullptr, nullptr};
        const WiFiClientFactory factory(&Config);
        const auto normalClient = factory.create(nullptr);
        const auto secureClient = factory.create(true);
        EXPECT_STREQ("WifiClient", normalClient->getType()) << "normalClient has type WifiClient";
        EXPECT_STREQ("WifiClientSecure", secureClient->getType()) << "secureClient has type SecureWifiClient";
        EXPECT_FALSE(dynamic_cast<WiFiClientSecure*>(secureClient)->isSecure()) << "Secure client without certs is not secure";
    }

    TEST(WifiClientFactoryTest, wifiClientFactorySecureClientTest) {
        constexpr TlsConfig Config{"a", "b", "c"};
        const WiFiClientFactory factory(&Config);
        const auto normalClient = factory.create("http://localhost");
        const auto secureClient = factory.create("https");
        EXPECT_STREQ("WifiClient", normalClient->getType()) << "Type is WifiClient";
        EXPECT_STREQ("WifiClientSecure", secureClient->getType()) << "Type is WifiClientSecure";
        EXPECT_TRUE(dynamic_cast<WiFiClientSecure*>(secureClient)->isSecure()) << "Secure client with certs is secure";
    }

}
