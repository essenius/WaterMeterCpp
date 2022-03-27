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
#include "../WaterMeterCpp/secrets.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {
    TEST_CLASS(ConfigurationTest) {
    public:
        TEST_METHOD(configurationTestLoadSecrets) {
            Preferences preferences;
            Configuration configuration(&preferences);
            configuration.begin();

            Assert::AreEqual(3741, configuration.freeBufferSpace(), L"Free buffer space OK");
            Assert::IsNotNull(configuration.mqtt.broker, L"Broker filled");
            Assert::IsNotNull(configuration.tls.rootCaCertificate, L"Root CA certificate filled");
            Assert::IsNotNull(configuration.wifi.ssid, L"SSID filled");
            Assert::IsNotNull(configuration.firmware.baseUrl, L"Firmware base URL filled");
        }

        TEST_METHOD(configurationTestMqttAndTls) {
            Preferences preferences;
            constexpr MqttConfig MQTT_CONFIG{"broker", 2048, "user", "password", false};
            constexpr TlsConfig TLS_CONFIG{"abc", "defg", "hijkl"};
            Configuration configuration(&preferences);
            configuration.putMqttConfig(&MQTT_CONFIG);
            configuration.putTlsConfig(&TLS_CONFIG);
            constexpr FirmwareConfig FIRMWARE_CONFIG{"http://localhost/firmware"};
            configuration.putFirmwareConfig(&FIRMWARE_CONFIG);

            configuration.begin(false);
            Assert::AreEqual("broker", configuration.mqtt.broker, L"Broker OK");
            Assert::AreEqual(2048u, configuration.mqtt.port, L"Port OK");
            Assert::AreEqual("user", configuration.mqtt.user, L"User OK");
            Assert::AreEqual("password", configuration.mqtt.password, L"Password OK");
            Assert::AreEqual<uint32_t>(0u, configuration.ip.localIp, L"IP 0");
            Assert::AreEqual<uint32_t>(0u, configuration.ip.secondaryDns, L"DNS2 0");
            Assert::AreEqual("abc", configuration.tls.rootCaCertificate, L"rootCA null");
            Assert::AreEqual("abc", configuration.tls.rootCaCertificate, L"rootCA ok");
            Assert::AreEqual("defg", configuration.tls.deviceCertificate, L"device cert ok");
            Assert::AreEqual("hijkl", configuration.tls.devicePrivateKey, L"device key ok");
            Assert::IsNull(configuration.wifi.deviceName, L"deviceName null");
            Assert::AreEqual("http://localhost/firmware", configuration.firmware.baseUrl, L"firmware url ok");
        }

        TEST_METHOD(configutationTestPutNullTest) {
            Preferences preferences;
            const Configuration configuration(&preferences);
            configuration.putIpConfig(nullptr);
            configuration.putMqttConfig(nullptr);
            configuration.putTlsConfig(nullptr);
            configuration.putWifiConfig(nullptr);
            configuration.putFirmwareConfig(nullptr);

            Assert::AreEqual<uint32_t>(0, configuration.ip.subnetMask, L"subnetmask not set");
            Assert::IsNull(configuration.mqtt.broker, L"Broker filled");
            Assert::IsNull(configuration.tls.rootCaCertificate, L"Root CA certificate filled");
            Assert::IsNull(configuration.wifi.ssid, L"SSID filled");
            Assert::IsNull(configuration.firmware.baseUrl, L"Firmware base URL filled");
        }

        TEST_METHOD(configurationTestWifiAndIp) {
            Preferences preferences;
            uint8_t bssidConfig[6] = {0, 1, 2, 3, 4, 5};
            const WifiConfig wifiConfig{"ssid", "password", "deviceName", bssidConfig};
            const IpConfig ipConfig{{1, 2, 3, 4}, {2, 3, 4, 5}, {3, 4, 5, 6}, {4, 5, 6, 7}, {5, 6, 7, 8}};
            Configuration configuration(&preferences);
            configuration.putWifiConfig(&wifiConfig);
            configuration.putIpConfig(&ipConfig);
            configuration.begin(false);
            Assert::AreEqual("ssid", configuration.wifi.ssid, L"SSID OK");
            Assert::AreEqual("password", configuration.wifi.password, L"Password OK");
            Assert::AreEqual("deviceName", configuration.wifi.deviceName, L"Device name OK");
            const auto bssid = configuration.wifi.bssid;
            Assert::IsNotNull(bssid, L"BSSID not null");
            for (unsigned int i = 0; i < sizeof bssidConfig; i++) {
                Assert::AreEqual(bssidConfig[i], bssid[i], L"bssid[i] ok");
            }
            Assert::AreEqual<uint32_t>(0x04030201, configuration.ip.localIp, L"localIP 0");
            Assert::AreEqual<uint32_t>(0x05040302, configuration.ip.gateway, L"gateway ok");
            Assert::AreEqual<uint32_t>(0x06050403, configuration.ip.subnetMask, L"subnet ok");
            Assert::AreEqual<uint32_t>(0x07060504, configuration.ip.primaryDns, L"DNS1 ok");
            Assert::AreEqual<uint32_t>(0x08070605, configuration.ip.secondaryDns, L"DNS2 ok");
            Assert::IsNull(configuration.tls.rootCaCertificate, L"rootCA null");
            Assert::IsNull(configuration.mqtt.broker, L"broker null");
            Assert::AreEqual(1883u, configuration.mqtt.port, L"port 1883");
        }
    };
}
