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

#ifndef HEADER_CONFIGURATION_H
#define HEADER_CONFIGURATION_H

#ifdef ESP32
#include <ESP.h>
#include <Preferences.h>
#else
#include "NetMock.h"
#include "PreferencesMock.h"
#endif

struct IpConfig {
    IPAddress localIp;
    IPAddress gateway;
    IPAddress subnetMask;
    IPAddress primaryDns;
    IPAddress secondaryDns;
};

const IPAddress NO_IP{0, 0, 0, 0};

// NO_IP means auto-configure
const IpConfig IP_AUTO_CONFIG{NO_IP, NO_IP, NO_IP, NO_IP, NO_IP};

struct FirmwareConfig {
    const char* baseUrl;
};

struct MqttConfig {
    const char* broker;
    unsigned int port;
    const char* user;
    const char* password;
};

struct TlsConfig {
    const char* rootCaCertificate;
    const char* deviceCertificate;
    const char* devicePrivateKey;
};

struct WifiConfig {
    const char* ssid;
    const char* password;
    const char* deviceName;
    uint8_t bssid[6]; // Format: { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 }. Use nullptr for autoconfigure
};


class Configuration {
public:
    explicit Configuration(Preferences* preferences);
    IpConfig ip{};
    MqttConfig mqtt{};
    TlsConfig tls{};
    WifiConfig wifi{};
    FirmwareConfig firmware{};
    void begin(bool useSecrets = true);
    void putFirmwareConfig(const FirmwareConfig* firnwareConfig) const;
    void putMqttConfig(const MqttConfig* mqttConfig) const;
    void putIpConfig(const IpConfig* ipConfig) const;
    void putTlsConfig(const TlsConfig* tlsConfig) const;
    void putWifiConfig(const WifiConfig* wifiConfig) const;
    int freeBufferSpace();
private:
    static constexpr int BUFFER_SIZE = 8192;
    Preferences* _preferences;
    char _buffer[8192] = {0};
    char* _next = _buffer;
    char* storeToBuffer(const char* key, char** startLocation);
    char* getFirmwareConfig(char* start);
    void getIpConfig();
    char* getMqttConfig(char* start);
    char* getTlsConfig(char* start);
    char* getWifiConfig(char* start);
    void putStringIfNotNull(const char* key, const char* value) const;
};

#endif
