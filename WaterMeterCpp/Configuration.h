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

// Grabs configuration data via the Preferences object. Can also initialize Preferences using a secrets.h file
// For security reasons this file is not checked in to source control, and can be removed once the preferences are stored.

#ifndef HEADER_CONFIGURATION
#define HEADER_CONFIGURATION

#include <IPAddress.h>
#include <Preferences.h>

struct IpConfig {
    IPAddress localIp;
    IPAddress gateway;
    IPAddress subnetMask;
    IPAddress primaryDns;
    IPAddress secondaryDns;
};

// INADDR_NONE means auto-configure
const IpConfig IpAutoConfig{INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE};

struct FirmwareConfig {
    const char* baseUrl;
};

struct MqttConfig {
    const char* broker;
    unsigned int port;
    const char* user;
    const char* password;
    bool useTls;
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
    uint8_t* bssid; // Format: { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 }. Use nullptr for autoconfigure
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
    int freeBufferSpace() const;
private:
    static constexpr int BufferSize = 8192;
    Preferences* _preferences;
    char _buffer[BufferSize] = {0};
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
