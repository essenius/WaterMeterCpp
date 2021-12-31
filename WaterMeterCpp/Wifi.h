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

#ifndef HEADER_WIFI_H
#define HEADER_WIFI_H

#include "EventClient.h"
#include "PayloadBuilder.h"

#ifdef ESP32
#include <WiFi.h>
#include <WiFiClientSecure.h>
#else
#include "NetMock.h"
#endif

class Wifi: public EventClient {

public:
    Wifi(EventServer* eventServer, const char* ssid, const char* password,
        const char* hostName = nullptr, const uint8_t* bssid = nullptr);
    void begin();
    void begin(IPAddress localIP, IPAddress gatewayIP = NO_IP, IPAddress subnetIP = NO_IP, IPAddress dns1IP = NO_IP, IPAddress dns2IP = NO_IP);
    const char* getHostName();
    const char* get(Topic topic, const char* defaultValue) override;
    void init();
    bool isConnected();
    void setCertificates(const char* rootCaCert, const char* deviceCert, const char* devicePrivateKey);
    const char* statusSummary();
    WiFiClientSecure* getClient();
    static const IPAddress NO_IP;


private:
    void completeConnection();

    static constexpr int HOSTNAME_LENGTH = 64;
    char _hostNameBuffer[HOSTNAME_LENGTH] = { 0 };
    const char* _ssid;     // not volatile
    const char* _password; // not volatile
    const uint8_t* _bssid; // not volatile
    PayloadBuilder _payloadBuilder;
    WiFiClientSecure _wifiClient;
    IPAddress _gatewayIP;
    IPAddress _netmaskIP;
    IPAddress _dns1IP;
    IPAddress _dns2IP;
    char* _hostName = _hostNameBuffer;
    static constexpr int MAC_ADDRESS_SIZE = 20;
    char _macAddress[MAC_ADDRESS_SIZE] = "";
    static constexpr int IP_ADDRESS_SIZE = 16;
    char _ipAddress[IP_ADDRESS_SIZE] = "";
    IPAddress _localIP = NO_IP;
};
#endif
