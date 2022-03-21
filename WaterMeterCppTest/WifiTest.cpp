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

#include <ESP.h>
#include "CppUnitTest.h"
#include "TestEventClient.h"
#include "../WaterMeterCpp/EventServer.h"
#include "../WaterMeterCpp/Wifi.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {

    TEST_CLASS(WifiTest) {
    public:
        static EventServer eventServer;
        static TestEventClient errorListener;

        TEST_CLASS_INITIALIZE(wifiTestClassInitialize) {
            eventServer.subscribe(&errorListener, Topic::ConnectionError);
        }

        TEST_METHOD_INITIALIZE(wifiTestMethodInitialize) {
            WiFi.reset();
            errorListener.reset();
        }

        TEST_CLASS_CLEANUP(wifiTestClassCleanup) {
            eventServer.unsubscribe(&errorListener);
        }

        TEST_METHOD(wifiAutomaticLocalIpTest) {
            const IPAddress local(10, 0, 0, 2);
            const IPAddress gateway(10, 0, 0, 1);
            const IPAddress dns1(8, 8, 8, 8);
            const IPAddress dns2(8, 8, 4, 4);
            constexpr WifiConfig WIFI_CONFIG{ "ssid", "password", "hostname", nullptr };
            PayloadBuilder payloadBuilder;
            TestEventClient client1(&eventServer);
            eventServer.subscribe(&client1, Topic::WifiSummaryReady);
            constexpr TlsConfig TLS_CONFIG{ nullptr, nullptr, nullptr};
            Wifi wifi(&eventServer, &WIFI_CONFIG, &payloadBuilder);
            wifi.begin();
            Assert::IsFalse(wifi.needsReinit(), L"Does not need reinit as disconnected");

            while (!wifi.isConnected()) {}
            Assert::IsTrue(wifi.needsReinit(), L"Needs reinit");
            wifi.begin();
            Assert::AreEqual(0, client1.getCallCount(), L"'Summary ready' not published yet");

            wifi.announceReady();
            Assert::AreEqual(1, client1.getCallCount(), L"'Summary ready' was published");
            Assert::AreEqual<uint32_t>(local, WiFi.localIP(), L"Local IP OK");
            Assert::AreEqual<uint32_t>(gateway, WiFi.gatewayIP(), L"Gateway IP OK");
            Assert::AreEqual<uint32_t>(IPAddress(255, 255, 0, 0), WiFi.subnetMask(), L"Subnet mask IP OK");
            Assert::AreEqual<uint32_t>(dns1, WiFi.dnsIP(), L"Primary DNS OK");
            Assert::AreEqual<uint32_t>(dns2, WiFi.dnsIP(1), L"Secondary DNS OK");
            Assert::AreEqual(
                R"({"ssid":"ssid","hostname":"hostname","mac-address":"00:11:22:33:44:55",)"
                R"("rssi-dbm":1,"channel":13,"network-id":"192.168.1.0","ip-address":"10.0.0.2",)"
                R"("gateway-ip":"10.0.0.1","dns1-ip":"8.8.8.8","dns2-ip":"8.8.4.4",)"
                R"("subnet-mask":"255.255.0.0","bssid":"55:44:33:22:11:00"})",
                payloadBuilder.toString(), L"Info OK");
        }

        TEST_METHOD(wifiFailSetNameTest) {
            constexpr WifiConfig CONFIG{ "ssid", "password", "", nullptr };
            PayloadBuilder payloadBuilder;
            constexpr TlsConfig TLS_CONFIG{ "a", "b", "c" };
            Wifi wifi(&eventServer, &CONFIG, &payloadBuilder);
            wifi.begin();
            // just showing intended usage
            Assert::AreEqual(1, errorListener.getCallCount(), L"Error called");
            Assert::AreEqual("Could not set host name", errorListener.getPayload(), L"Error message OK");
        }

        TEST_METHOD(wifiGetUnknownTopicTestTest) {

            constexpr WifiConfig CONFIG{ "ssid", "password", "hostname", nullptr };
            PayloadBuilder payloadBuilder;
            constexpr TlsConfig TLS_CONFIG{ nullptr, nullptr, nullptr };
            Wifi wifi(&eventServer, &CONFIG, &payloadBuilder);
            Assert::AreEqual("x", wifi.get(Topic::Flow, "x"), L"Unexpected topic returns default");
        }

        TEST_METHOD(wifiNullNameTest) {
            WiFi.setHostname("esp32_001122334455");
            constexpr WifiConfig CONFIG{ "ssid", "password", nullptr, nullptr };
            PayloadBuilder payloadBuilder;

            constexpr TlsConfig TLS_CONFIG{ nullptr, nullptr, nullptr };
            Wifi wifi(&eventServer, &CONFIG, &payloadBuilder);
            wifi.begin();

            Assert::AreEqual(0, errorListener.getCallCount(), L"Error not called");
            Assert::AreEqual("esp32_001122334455", WiFi.getHostname(), L"Default hostname set");
            WiFi.connectIn(1);
            while (!wifi.isConnected()) {}
            wifi.disconnect();
            Assert::IsFalse(wifi.isConnected(), L"Disconnected");
            wifi.reconnect();
            while (!wifi.isConnected()) {}
        }

        TEST_METHOD(wifiPredefinedLocalIpTest) {
            const IPAddress local(192, 168, 1, 2);
            const IPAddress gateway(192, 168, 1, 1);
            constexpr WifiConfig WIFI_CONFIG{"ssid", "password", "hostname", nullptr };
            PayloadBuilder payloadBuilder;
            constexpr TlsConfig TLS_CONFIG{ nullptr, nullptr, nullptr };
            Wifi wifi(&eventServer, &WIFI_CONFIG, &payloadBuilder);
            const IpConfig ipConfig{local, NO_IP, NO_IP, NO_IP, NO_IP};
            wifi.configure(&ipConfig);
            wifi.begin();
            Assert::IsFalse(wifi.needsReinit(), L"Does not need reinit");
            wifi.announceReady();
            Assert::AreEqual(local.toString().c_str(), WiFi.localIP().toString().c_str(), L"Local IP OK");
            Assert::AreEqual(gateway.toString().c_str(), WiFi.gatewayIP().toString().c_str(), L"Gateway IP OK");
            Assert::AreEqual<uint32_t>(IPAddress(255, 255, 255, 0), WiFi.subnetMask(), L"Subnet mask IP OK");
            Assert::AreEqual<uint32_t>(gateway, WiFi.dnsIP(), L"Primary DNS OK");
            Assert::AreEqual<uint32_t>(gateway, WiFi.dnsIP(1), L"Secondary DNS OK");
            Assert::AreEqual(
                R"({"ssid":"ssid","hostname":"hostname","mac-address":"00:11:22:33:44:55",)"
                R"("rssi-dbm":1,"channel":13,"network-id":"192.168.1.0","ip-address":"192.168.1.2",)"
                R"("gateway-ip":"192.168.1.1","dns1-ip":"192.168.1.1","dns2-ip":"192.168.1.1",)"
                R"("subnet-mask":"255.255.255.0","bssid":"55:44:33:22:11:00"})",
                payloadBuilder.toString(),
                L"Info OK");
            Assert::AreEqual("001122334455", wifi.get(Topic::MacRaw, ""), L"Mac address for URL ok");
            Assert::AreEqual(
                "00:11:22:33:44:55", eventServer.request(Topic::MacFormatted, ""),
                L"Mac address formatted ok");
            Assert::AreEqual("192.168.1.2", wifi.get(Topic::IpAddress, ""), L"IP address ok2");
        }

        TEST_METHOD(wifiPredefinedPrimaryDnsTest) {
            const IPAddress local(192, 168, 1, 2);
            const IPAddress gateway(192, 168, 1, 253);
            const IPAddress dns(9, 9, 9, 9);
            constexpr WifiConfig CONFIG{"ssid", "password", "hostname", nullptr };
            PayloadBuilder payloadBuilder;

            Wifi wifi(&eventServer, &CONFIG, &payloadBuilder);
            const IpConfig ipConfig{local, gateway, NO_IP, dns, NO_IP};
            wifi.configure(&ipConfig);
            wifi.begin();
            Assert::IsFalse(wifi.needsReinit(), L"Does not need reinit");
            wifi.announceReady();
            Assert::AreEqual<uint32_t>(local, WiFi.localIP(), L"Local IP OK");
            Assert::AreEqual<uint32_t>(gateway, WiFi.gatewayIP(), L"Gateway IP OK");
            Assert::AreEqual<uint32_t>(dns, WiFi.dnsIP(), L"Primary DNS OK");
            Assert::AreEqual<uint32_t>(dns, WiFi.dnsIP(1), L"Secondary DNS OK");
            Assert::AreEqual(
                R"({"ssid":"ssid","hostname":"hostname","mac-address":"00:11:22:33:44:55",)"
                R"("rssi-dbm":1,"channel":13,"network-id":"192.168.1.0","ip-address":"192.168.1.2",)"
                R"("gateway-ip":"192.168.1.253","dns1-ip":"9.9.9.9","dns2-ip":"9.9.9.9",)"
                R"("subnet-mask":"255.255.255.0","bssid":"55:44:33:22:11:00"})",
                payloadBuilder.toString(),
                L"Info OK");
        }

        TEST_METHOD(wifiPredefinedSecondaryDnsTest) {
            const IPAddress local(192, 168, 1, 2);
            const IPAddress gateway(192, 168, 1, 1);
            const IPAddress dns1(9, 9, 9, 9);
            const IPAddress dns2(1, 1, 1, 1);

            constexpr WifiConfig CONFIG{"ssid", "password", "hostname", nullptr };
            PayloadBuilder payloadBuilder;

            Wifi wifi(&eventServer, &CONFIG, &payloadBuilder);
            const IpConfig ipConfig{local, NO_IP, NO_IP, dns1, dns2};
            wifi.configure(&ipConfig);
            wifi.begin();
            Assert::IsFalse(wifi.needsReinit(), L"does not need reinit");
            Assert::AreEqual(gateway.toString().c_str(), WiFi.gatewayIP().toString().c_str(), L"Gateway IP OK");
            Assert::AreEqual<uint32_t>(dns1, WiFi.dnsIP(), L"Primary DNS OK");
            Assert::AreEqual<uint32_t>(dns2, WiFi.dnsIP(1), L"Secondary DNS OK");
            wifi.announceReady();
            Assert::AreEqual(
                R"({"ssid":"ssid","hostname":"hostname","mac-address":"00:11:22:33:44:55",)"
                R"("rssi-dbm":1,"channel":13,"network-id":"192.168.1.0","ip-address":"192.168.1.2",)"
                R"("gateway-ip":"192.168.1.1","dns1-ip":"9.9.9.9","dns2-ip":"1.1.1.1",)"
                R"("subnet-mask":"255.255.255.0","bssid":"55:44:33:22:11:00"})",
                payloadBuilder.toString(),
                L"Info OK");
        }
    };

    EventServer WifiTest::eventServer;
    TestEventClient WifiTest::errorListener(&eventServer);
}
