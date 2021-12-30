// Copyright 2021 Rik Essenius
// 
//   Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//   except in compliance with the License. You may obtain a copy of the License at
// 
//       http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software distributed under the License
//    is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and limitations under the License.

#include "pch.h"

#include "CppUnitTest.h"
#include "TestEventClient.h"
#include "../WaterMeterCpp/EventServer.h"
#include "../WaterMeterCpp/ArduinoMock.h"
#include "../WaterMeterCpp/Wifi.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace WaterMeterCppTest {

	EventServer eventServer;
	TestEventClient wifiConnectingListener("connect", &eventServer);
	TestEventClient wifiErrorListener("error", &eventServer);
	TestEventClient wifiInfoListener("info", &eventServer);

	TEST_CLASS(WifiTest) {
public:

	    TEST_CLASS_INITIALIZE(wifiTestClassInitialize) {
			eventServer.subscribe(&wifiConnectingListener, Topic::Connecting);
			eventServer.subscribe(&wifiErrorListener, Topic::Error);
			eventServer.subscribe(&wifiInfoListener, Topic::Info);

	    }
		TEST_METHOD_INITIALIZE(wifiTestMethodInitialize) {
			WiFi.reset();
			wifiConnectingListener.reset();
			wifiErrorListener.reset();
			wifiInfoListener.reset();
		}

		TEST_CLASS_CLEANUP(wifiTestClassCleanup) {
			eventServer.unsubscribe(&wifiConnectingListener);
			eventServer.unsubscribe(&wifiInfoListener);
			eventServer.unsubscribe(&wifiErrorListener);
	    }
	TEST_METHOD(wifiPredefinedLocalIpTest) {
		const IPAddress local(192, 168, 1, 2);
        const IPAddress gateway(192, 168, 1, 1);
		Wifi wifi(&eventServer, "ssid", "password", "hostname");
		wifi.begin(local);
		Assert::AreEqual(9, wifiConnectingListener.getCallCount(), L"Connecting called 9 times");
		Assert::AreEqual(local.toString().c_str(), WiFi.localIP().toString().c_str(), L"Local IP OK");
		Assert::AreEqual(gateway.toString().c_str(), WiFi.gatewayIP().toString().c_str(), L"Gateway IP OK");
		Assert::AreEqual<uint32_t>(IPAddress(255, 255, 255, 0), WiFi.subnetMask(), L"Subnet mask IP OK");
		Assert::AreEqual<uint32_t>(gateway, WiFi.dnsIP(), L"Primary DNS OK");
		Assert::AreEqual<uint32_t>(gateway, WiFi.dnsIP(1), L"Secondary DNS OK");
		Assert::AreEqual(1, wifiInfoListener.getCallCount(), L"Info called 1 time");
		Assert::AreEqual(
			R"({"ssid":"ssid","hostname":"hostname","mac-address":"00:11:22:33:44:55",)"
			R"("rssi-dbm":1,"channel":13,"network-id":"192.168.1.0","ip-address":"192.168.1.2",)"
			R"("gateway-ip":"192.168.1.1","dns1-ip":"192.168.1.1","dns2-ip":"192.168.1.1",)"
			R"("subnet-mask":"255.255.255.0","bssid":"55:44:33:22:11:00"})", 
			wifiInfoListener.getPayload(), 
			L"Info OK");
		Assert::AreEqual("001122334455", wifi.macAddress(), "Mac address for URL ok");

	}

	TEST_METHOD(wifiAutomaticLocalIpTest) {
		const IPAddress local(10, 0, 0, 2);
		const IPAddress gateway(10, 0, 0, 1);
		const IPAddress dns1(8, 8, 8, 8);
		const IPAddress dns2(8, 8, 4, 4);
		Wifi wifi(&eventServer, "ssid", "password", "hostname");
		wifi.begin();
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
			wifiInfoListener.getPayload(),
			L"Info OK");
	}

		// TODO: this is a bit weird as it fails two times. One time should be enough
	TEST_METHOD(wifiFailSetNameTest) {
		const IPAddress local(10, 0, 0, 2);
		const IPAddress gateway(10, 0, 0, 1);
		const IPAddress dns1(8, 8, 8, 8);
		const IPAddress dns2(8, 8, 4, 4);
		Wifi wifi(&eventServer, "ssid", "password", "");
		wifi.begin();
		// just showing intended usage
		wifi.setCertificates("", "", "");
		Assert::AreEqual(2, wifiErrorListener.getCallCount(), L"Error called 2 times");
		Assert::AreEqual("Could not set host name", wifiErrorListener.getPayload(), L"Error message OK");

	}
	TEST_METHOD(wifiNullNameTest) {
		WiFi.setHostname("esp32_001122334455");

		Wifi wifi(&eventServer, "ssid", "password", nullptr);
		wifi.begin();
		Assert::AreEqual(0, wifiErrorListener.getCallCount(), L"Error not called");
		Assert::AreEqual("esp32_001122334455", WiFi.getHostname(), L"Default hostname set");
	}


	TEST_METHOD(wifiPredefinedPrimaryDnsTest) {
		const IPAddress local(192, 168, 1, 2);
		const IPAddress gateway(192, 168, 1, 1);
		const IPAddress dns(9, 9, 9, 9);
		Wifi wifi(&eventServer, "ssid", "password", "hostname");
		wifi.begin(local, Wifi::NO_IP, Wifi::NO_IP, dns);
		Assert::AreEqual(local.toString().c_str(), WiFi.localIP().toString().c_str(), L"Local IP OK");
		Assert::AreEqual(gateway.toString().c_str(), WiFi.gatewayIP().toString().c_str(), L"Gateway IP OK");
		Assert::AreEqual<uint32_t>(dns, WiFi.dnsIP(), L"Primary DNS OK");
		Assert::AreEqual<uint32_t>(dns, WiFi.dnsIP(1), L"Secondary DNS OK");
		Assert::AreEqual(1, wifiInfoListener.getCallCount(), L"Info called 1 time");
		Assert::AreEqual(
			R"({"ssid":"ssid","hostname":"hostname","mac-address":"00:11:22:33:44:55",)"
			R"("rssi-dbm":1,"channel":13,"network-id":"192.168.1.0","ip-address":"192.168.1.2",)"
			R"("gateway-ip":"192.168.1.1","dns1-ip":"9.9.9.9","dns2-ip":"9.9.9.9",)"
			R"("subnet-mask":"255.255.255.0","bssid":"55:44:33:22:11:00"})",
			wifiInfoListener.getPayload(),
			L"Info OK");
	}

	TEST_METHOD(wifiPredefinedSecondaryDnsTest) {
		const IPAddress local(192, 168, 1, 2);
		const IPAddress gateway(192, 168, 1, 1);
		const IPAddress dns1(9, 9, 9, 9);
		const IPAddress dns2(1, 1, 1, 1);

		Wifi wifi(&eventServer, "ssid", "password", "hostname");
		wifi.begin(local, Wifi::NO_IP, Wifi::NO_IP, dns1, dns2);
		Assert::AreEqual(gateway.toString().c_str(), WiFi.gatewayIP().toString().c_str(), L"Gateway IP OK");
		Assert::AreEqual<uint32_t>(dns1, WiFi.dnsIP(), L"Primary DNS OK");
		Assert::AreEqual<uint32_t>(dns2, WiFi.dnsIP(1), L"Secondary DNS OK");
		Assert::AreEqual(1, wifiInfoListener.getCallCount(), L"Info called 1 time");
		Assert::AreEqual(
			R"({"ssid":"ssid","hostname":"hostname","mac-address":"00:11:22:33:44:55",)"
			R"("rssi-dbm":1,"channel":13,"network-id":"192.168.1.0","ip-address":"192.168.1.2",)"
			R"("gateway-ip":"192.168.1.1","dns1-ip":"9.9.9.9","dns2-ip":"1.1.1.1",)"
			R"("subnet-mask":"255.255.255.0","bssid":"55:44:33:22:11:00"})",
			wifiInfoListener.getPayload(),
			L"Info OK");
	}

	};
}