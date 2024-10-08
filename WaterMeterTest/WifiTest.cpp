﻿// Copyright 2021-2024 Rik Essenius
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

#include <ESP.h>
#include <IPAddress.h>

#include "CppUnitTest.h"
#include "TestEventClient.h"
#include "WiFi.h"
#include "EventServer.h"
#include "WiFiManager.h"

namespace WaterMeterCppTest {
    using WaterMeter::IpConfig;
    using WaterMeter::NoIp;
    using WaterMeter::PayloadBuilder;
    using WaterMeter::WifiConfig;
    using WaterMeter::WiFiManager;

    class WiFiTest : public testing::Test {
    public:
        static EventServer eventServer;
        static TestEventClient errorListener;

        // ReSharper disable once CppInconsistentNaming
        static void SetUpTestCase() {
            eventServer.subscribe(&errorListener, Topic::ConnectionError);
        }

        void SetUp() override {
            WiFi.reset();
            errorListener.reset();
        }

        // ReSharper disable once CppInconsistentNaming
        static void TearDownTestCase() {
            eventServer.unsubscribe(&errorListener);
        }
    };

    EventServer WiFiTest::eventServer;
    TestEventClient WiFiTest::errorListener(&eventServer);

    void expectWiFiIp(IPAddress local, IPAddress gateway, IPAddress dns1, IPAddress dns2) {
        EXPECT_EQ(local, WiFi.localIP()) << "Local IP OK";
        EXPECT_EQ(gateway, WiFi.gatewayIP()) << "Gateway IP OK";
        EXPECT_EQ(dns1, WiFi.dnsIP()) << "Primary DNS OK";
        EXPECT_EQ(dns2, WiFi.dnsIP(1)) << "Secondary DNS OK";
    }

    TEST_F(WiFiTest, automaticLocalIpTest) {
        const IPAddress local(10, 0, 0, 2);
        const IPAddress gateway(10, 0, 0, 1);
        const IPAddress dns1(8, 8, 8, 8);
        const IPAddress dns2(8, 8, 4, 4);
        constexpr WifiConfig WifiConfig{"ssid", "password", "hostname", nullptr};
        PayloadBuilder payloadBuilder;
        TestEventClient client1(&eventServer);
        eventServer.subscribe(&client1, Topic::WifiSummaryReady);
        WiFiManager wifi(&eventServer, &WifiConfig, &payloadBuilder);
        wifi.begin();
        EXPECT_FALSE(wifi.needsReInit()) << "Does not need reInit as disconnected";

        while (!wifi.isConnected()) {}
        EXPECT_TRUE(wifi.needsReInit()) << "Needs reInit";
        wifi.begin();
        EXPECT_EQ(0, client1.getCallCount()) << "'Summary ready' not published yet";

        wifi.announceReady();
        EXPECT_EQ(1, client1.getCallCount()) << "'Summary ready' was published";

        expectWiFiIp(local, gateway, dns1, dns2);

        EXPECT_EQ(IPAddress(255, 255, 0, 0), WiFi.subnetMask()) << "Subnet mask IP OK";

        EXPECT_STREQ(
            R"({"ssid":"ssid","hostname":"hostname","mac-address":"00:11:22:33:44:55",)"
            R"("rssi-dbm":1,"channel":13,"network-id":"192.168.1.0","ip-address":"10.0.0.2",)"
            R"("gateway-ip":"10.0.0.1","dns1-ip":"8.8.8.8","dns2-ip":"8.8.4.4",)"
            R"("subnet-mask":"255.255.0.0","bssid":"55:44:33:22:11:00"})",
            payloadBuilder.toString()) << "Info OK";
    }

    TEST_F(WiFiTest,failSetNameTest) {
        constexpr WifiConfig Config{"ssid", "password", "", nullptr};
        PayloadBuilder payloadBuilder;
        WiFiManager wifi(&eventServer, &Config, &payloadBuilder);
        wifi.begin();
        // just showing intended usage
        EXPECT_EQ(1, errorListener.getCallCount()) << "Error called";
        EXPECT_STREQ("Could not set host name", errorListener.getPayload()) << "Error message OK";
    }

    TEST_F(WiFiTest, getUnknownTopicTestTest) {
        constexpr WifiConfig Config{"ssid", "password", "hostname", nullptr};
        PayloadBuilder payloadBuilder;
        WiFiManager wifi(&eventServer, &Config, &payloadBuilder);
        EXPECT_STREQ("x", wifi.get(Topic::Pulse, "x")) << "Unexpected topic returns default";
    }

    TEST_F(WiFiTest, nullNameTest) {
        WiFi.setHostname("esp32_001122334455");
        constexpr WifiConfig Config{"ssid", "password", nullptr, nullptr};
        PayloadBuilder payloadBuilder;

        WiFiManager wifi(&eventServer, &Config, &payloadBuilder);
        wifi.begin();

        EXPECT_EQ(0, errorListener.getCallCount()) << "Error not called";
        EXPECT_STREQ("esp32_001122334455", WiFi.getHostname()) << "Default hostname set";
        WiFi.connectIn(1);
        while (!wifi.isConnected()) {}
        wifi.disconnect();
        EXPECT_FALSE(wifi.isConnected()) << "Disconnected";
        wifi.reconnect();
        while (!wifi.isConnected()) {}
    }

    TEST_F(WiFiTest, predefinedLocalIpTest) {
        const IPAddress local(192, 168, 1, 2);
        const IPAddress gateway(192, 168, 1, 1);
        constexpr WifiConfig WifiConfig{"ssid", "password", "hostname", nullptr};
        PayloadBuilder payloadBuilder;
        WiFiManager wifi(&eventServer, &WifiConfig, &payloadBuilder);
        const IpConfig ipConfig{local, NoIp, NoIp, NoIp, NoIp};
        wifi.configure(&ipConfig);
        wifi.begin();
        EXPECT_FALSE(wifi.needsReInit()) << "Does not need reInit";
        wifi.announceReady();
        expectWiFiIp(local, gateway, gateway, gateway);
        EXPECT_EQ(IPAddress(255, 255, 255, 0), WiFi.subnetMask()) << "Subnet mask IP OK";
        EXPECT_STREQ(
            R"({"ssid":"ssid","hostname":"hostname","mac-address":"00:11:22:33:44:55",)"
            R"("rssi-dbm":1,"channel":13,"network-id":"192.168.1.0","ip-address":"192.168.1.2",)"
            R"("gateway-ip":"192.168.1.1","dns1-ip":"192.168.1.1","dns2-ip":"192.168.1.1",)"
            R"("subnet-mask":"255.255.255.0","bssid":"55:44:33:22:11:00"})",
            payloadBuilder.toString()) << "Info OK";
        EXPECT_STREQ("001122334455", wifi.get(Topic::MacRaw, "")) << "Mac address for URL ok";
        EXPECT_STREQ("00:11:22:33:44:55", eventServer.request(Topic::MacFormatted, "")) << L"Mac address formatted ok";
        EXPECT_STREQ("192.168.1.2", wifi.get(Topic::IpAddress, "")) << "IP address ok2";
    }

    TEST_F(WiFiTest, predefinedPrimaryDnsTest) {
        const IPAddress local(192, 168, 1, 2);
        const IPAddress gateway(192, 168, 1, 253);
        const IPAddress dns(9, 9, 9, 9);
        constexpr WifiConfig Config{"ssid", "password", "hostname", nullptr};
        PayloadBuilder payloadBuilder;

        WiFiManager wifi(&eventServer, &Config, &payloadBuilder);
        const IpConfig ipConfig{local, gateway, NoIp, dns, NoIp};
        wifi.configure(&ipConfig);
        wifi.begin();
        EXPECT_FALSE(wifi.needsReInit()) << "Does not need reInit";
        wifi.announceReady();
        expectWiFiIp(local, gateway, dns, dns);
        EXPECT_STREQ(
            R"({"ssid":"ssid","hostname":"hostname","mac-address":"00:11:22:33:44:55",)"
            R"("rssi-dbm":1,"channel":13,"network-id":"192.168.1.0","ip-address":"192.168.1.2",)"
            R"("gateway-ip":"192.168.1.253","dns1-ip":"9.9.9.9","dns2-ip":"9.9.9.9",)"
            R"("subnet-mask":"255.255.255.0","bssid":"55:44:33:22:11:00"})",
            payloadBuilder.toString()) << "Info OK";
    }

    TEST_F(WiFiTest, predefinedSecondaryDnsTest) {
        const IPAddress local(192, 168, 1, 2);
        const IPAddress gateway(192, 168, 1, 1);
        const IPAddress dns1(9, 9, 9, 9);
        const IPAddress dns2(1, 1, 1, 1);

        constexpr WifiConfig Config{"ssid", "password", "hostname", nullptr};
        PayloadBuilder payloadBuilder;

        WiFiManager wifi(&eventServer, &Config, &payloadBuilder);
        const IpConfig ipConfig{local, NoIp, NoIp, dns1, dns2};
        wifi.configure(&ipConfig);
        wifi.begin();
        EXPECT_FALSE(wifi.needsReInit()) << "does not need reInit";
        EXPECT_STREQ(gateway.toString().c_str(), WiFi.gatewayIP().toString().c_str()) << "Gateway IP OK";
        EXPECT_EQ(dns1, WiFi.dnsIP()) << "Primary DNS OK";
        EXPECT_EQ(dns2, WiFi.dnsIP(1)) << "Secondary DNS OK";
        wifi.announceReady();
        EXPECT_STREQ(
            R"({"ssid":"ssid","hostname":"hostname","mac-address":"00:11:22:33:44:55",)"
            R"("rssi-dbm":1,"channel":13,"network-id":"192.168.1.0","ip-address":"192.168.1.2",)"
            R"("gateway-ip":"192.168.1.1","dns1-ip":"9.9.9.9","dns2-ip":"1.1.1.1",)"
            R"("subnet-mask":"255.255.255.0","bssid":"55:44:33:22:11:00"})",
            payloadBuilder.toString()) << "Info OK";
    }
}
