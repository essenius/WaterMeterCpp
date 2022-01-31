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

#ifndef CONFIG_H
#define CONFIG_H

#ifdef ESP32
#include <ESP.h>
#else
#define PROGMEM
#include "NetMock.h"
#endif

struct WifiConfig {
    const char* ssid;
    const char* password;
    const char* deviceName;
    const uint8_t* bssid; // Format: { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 }. Use nullptr for autoconfigure
};

struct MqttConfig {
    const char* broker;
    const int port;
    const char* user;
    const char* password;
};

struct IpConfig {
    const IPAddress localIp;
    const IPAddress gateway;
    const IPAddress subnetMask;
    const IPAddress primaryDns;
    const IPAddress secondaryDns;
};

const IPAddress NO_IP{0, 0, 0, 0};

// NO_IP means auto-configure

const IpConfig IP_AUTO_CONFIG{NO_IP, NO_IP, NO_IP, NO_IP, NO_IP};

#define CONFIG_USE_TLS

struct TlsConfig {
    const char* rootCaCertificate;
    const char* deviceCertificate;
    const char* devicePrivateKey;
};

#endif
