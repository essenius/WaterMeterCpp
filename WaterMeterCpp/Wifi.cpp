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

#ifdef ESP32
#include <HTTPClient.h>
#else
#include <string.h>
#endif

#include "Wifi.h"
#include "secrets_wifi.h"

WiFiClientSecure* Wifi::getClient() {
    return &_wifiClient;
}

void Wifi::begin() {
    WiFi.mode(WIFI_STA);
    // There is an issue on ESP32 with DHCP, timing out after 12032 seconds.
    // Workaround is setting a fixed IP address so we don't need DHCP
#ifdef CONFIG_LOCAL_IP
    IPAddress localIP(CONFIG_LOCAL_IP);
    IPAddress gateway(CONFIG_GATEWAY);
    IPAddress primaryDNS(CONFIG_PRIMARY_DNS);
#ifdef CONFIG_SUBNET
    IPAddress subnet(CONFIG_SUBNET);
#else
    IPAddress subnet(255, 255, 255, 0);
#endif     
    if (!WiFi.config(localIP, gateway, subnet, primaryDNS)) {
        Serial.println("Could not configure Wifi with static IP");
    }
#endif
    // if a specific access point was specified, honor that
#ifdef CONFIG_BSSID
    byte bssid[] = { CONFIG_BSSID };
    WiFi.begin(CONFIG_SSID, CONFIG_PASSWORD, 0, bssid);
#else
    WiFi.begin(CONFIG_SSID, CONFIG_PASSWORD);
#endif

    Serial.print("Connecting");
    while (!WiFi.isConnected()) {
        delay(100);
        Serial.print(".");
    }
    Serial.println("\nConnected to Wifi");
    _wifiClient.setCACert(CONFIG_ROOTCA_CERTIFICATE);
    _wifiClient.setCertificate(CONFIG_DEVICE_CERTIFICATE);
    _wifiClient.setPrivateKey(CONFIG_DEVICE_PRIVATE_KEY);

    if (!WiFi.setHostname(CONFIG_DEVICE_NAME)) {
        Serial.println("Could not set host name");
    }
    else {
        Serial.printf("Set hostname to '%s'\n", WiFi.getHostname());
    }
    strcpy(_hostName, WiFi.getHostname());
    Serial.println(statusSummary());
}

const char* Wifi::getHostName() {
    return _hostName;
}

const char* Wifi::statusSummary() {
    _payloadBuilder.initialize();
    _payloadBuilder.writeParam("ssid", WiFi.SSID().c_str());
    _payloadBuilder.writeParam("hostname", getHostName());
    _payloadBuilder.writeParam("mac-address", WiFi.macAddress().c_str());
    _payloadBuilder.writeParam("rssi-dbm", WiFi.RSSI());
    _payloadBuilder.writeParam("channel", WiFi.channel());
    _payloadBuilder.writeParam("network-id", WiFi.networkID().toString().c_str());
    _payloadBuilder.writeParam("ip-address", WiFi.localIP().toString().c_str());
    _payloadBuilder.writeParam("gateway-ip", WiFi.gatewayIP().toString().c_str());
    _payloadBuilder.writeParam("dns-ip", WiFi.dnsIP().toString().c_str());
    _payloadBuilder.writeParam("subnet-mask", WiFi.subnetMask().toString().c_str());
    _payloadBuilder.writeParam("bssid", WiFi.BSSIDstr().c_str());
    _payloadBuilder.writeGroupEnd();
    return _payloadBuilder.toString();
}

bool Wifi::isConnected() {
    return WiFi.isConnected();
}

const char* Wifi::macAddress() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  sprintf(_macAddress, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]); 
  return _macAddress; 
}
