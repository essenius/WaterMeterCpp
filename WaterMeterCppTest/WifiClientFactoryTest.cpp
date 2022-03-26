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
#include "../WaterMeterCpp/WiFiClientFactory.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {

    TEST_CLASS(WifiClientFactoryTest) {
public:

    TEST_METHOD(wifiClientFactoryInsecureClientTest) {
        constexpr TlsConfig CONFIG{ nullptr, nullptr, nullptr };
        const WiFiClientFactory factory(&CONFIG);
        const auto normalClient = factory.create(nullptr);
        const auto secureClient = factory.create(true);
        Assert::AreEqual("WifiClient", normalClient->getType(), L"normalClient has type WifiClient");
        Assert::AreEqual("WifiClientSecure", secureClient->getType(), L"secureClient has type SecureWifiClient");
        Assert::IsFalse(dynamic_cast<WiFiClientSecure*>(secureClient)->isSecure());
    }

    TEST_METHOD(wifiClientFactorySecureClientTest) {
        constexpr TlsConfig CONFIG{ "a", "b", "c" };
        const WiFiClientFactory factory(&CONFIG);
        const auto normalClient = factory.create("http://localhost");
        const auto secureClient = factory.create("https");
        Assert::AreEqual("WifiClient", normalClient->getType());
        Assert::AreEqual("WifiClientSecure", secureClient->getType());
        Assert::IsTrue(dynamic_cast<WiFiClientSecure*>(secureClient)->isSecure());
    }

    };
}
