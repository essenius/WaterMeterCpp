// Copyright 2021-2022 Rik Essenius
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
#include "config.h"

#ifdef ESP32
#include <WiFi.h>
#include <WiFiClientSecure.h>
#else
#include "NetMock.h"
#endif

class Wifi : public EventClient {

public:
    Wifi(EventServer* eventServer, const WifiConfig* wifiConfig, PayloadBuilder* payloadBuilder);
    virtual void announceReady();
    virtual void begin();
    void configure(const IpConfig* ipConfig = &IP_AUTO_CONFIG);
    virtual void disconnect();
    const char* get(Topic topic, const char* defaultValue) override;
    WiFiClient* getClient();
    const char* getHostName() const;
    virtual bool isConnected();
    virtual bool needsReinit();
    virtual void reconnect();
    void setCertificates(const char* rootCaCertificate, const char* deviceCertificate, const char* devicePrivateKey);
    void setStatusSummary() const;

private:
    static constexpr int HOSTNAME_LENGTH = 64;
    static constexpr int MAC_ADDRESS_SIZE = 20;
    static constexpr int IP_ADDRESS_SIZE = 16;

    PayloadBuilder* _payloadBuilder;
    const WifiConfig* _wifiConfig;
    WiFiClientSecure _wifiClient;
    IPAddress _localIp = NO_IP;
    IPAddress _gatewayIp = NO_IP;
    IPAddress _subnetMaskIp = NO_IP;
    IPAddress _dns1Ip = NO_IP;
    IPAddress _dns2Ip = NO_IP;
    char _hostNameBuffer[HOSTNAME_LENGTH] = { 0 };
    char* _hostName = _hostNameBuffer;
    char _ipAddress[IP_ADDRESS_SIZE] = "";
    char _macAddress[MAC_ADDRESS_SIZE] = "";
    bool _needsReconnect = true;
};
#endif
