// Copyright 2021-2023 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

// Sets up and maintains the Wifi connection, and provides details that the communicator will need (e.g. IP address, MAC Address).

#ifndef HEADER_WIFIMANAGER
#define HEADER_WIFIMANAGER

#include <IPAddress.h>
#include <WiFiClientSecure.h>

#include "EventClient.h"
#include "PayloadBuilder.h"
#include "Configuration.h"

namespace WaterMeter {
    const IPAddress NoIp(0, 0, 0, 0);

    class WiFiManager : public EventClient {
    public:
        WiFiManager(EventServer* eventServer, const WifiConfig* wifiConfig, PayloadBuilder* payloadBuilder);
        virtual void announceReady();
        virtual void begin();
        void configure(const IpConfig* ipConfig = &IpAutoConfig);
        virtual void disconnect();
        const char* get(Topic topic, const char* defaultValue) override;
        const char* getHostName() const;
        virtual bool isConnected();
        virtual bool needsReinit();
        virtual void reconnect();
        void setStatusSummary() const;

    private:
        static constexpr int HostnameLength = 64;
        static constexpr int MacAddressSize = 20;
        static constexpr int IpAddressSize = 16;

        PayloadBuilder* _payloadBuilder;
        const WifiConfig* _wifiConfig;
        WiFiClientSecure _wifiClient;
        IPAddress _localIp = NoIp;
        IPAddress _gatewayIp = NoIp;
        IPAddress _subnetMaskIp = NoIp;
        IPAddress _dns1Ip = NoIp;
        IPAddress _dns2Ip = NoIp;
        char _hostNameBuffer[HostnameLength] = { 0 };
        char* _hostName = _hostNameBuffer;
        char _ipAddress[IpAddressSize] = "";
        char _macAddress[MacAddressSize] = "";
        bool _needsReconnect = true;
    };
}
#endif
