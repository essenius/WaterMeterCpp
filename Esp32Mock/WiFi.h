// Copyright 2022 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

// Mock implementation of the WiFi class/object for unit testing (not targeting the ESP32)

// Disabling warnings caused by mimicking existing interface
// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppMemberFunctionMayBeStatic
// ReSharper disable CppInconsistentNaming
// ReSharper disable CppParameterNeverUsed
// ReSharper disable CppParameterMayBeConst

#ifndef HEADER_WIFI
#define HEADER_WIFI

#include "ESP.h"
#include "IPAddress.h"
#include "StringArduino.h"

class WiFiClass {
public:
    WiFiClass();

    void mode(int i) { }

    void begin(const char* ssid, const char* password, int ignore = 0, const uint8_t* _bssid = nullptr) {
        SafeCString::strcpy(_ssid, ssid);
    }

    bool config(IPAddress local, IPAddress gateway, IPAddress subnet,
                IPAddress dns1 = IPAddress(), IPAddress dns2 = IPAddress());
    String BSSIDstr() { return {"55:44:33:22:11:00"}; }
    int channel() { return 13; }
    void disconnect() { _connectCountdown = _connectMax; }
    IPAddress dnsIP(int i = 0) { return i == 0 ? _primaryDNSIP : _secondaryDNSIP; }
    IPAddress gatewayIP() { return _gatewayIP; }
    const char* getHostname() { return _name; }
    bool isConnected();
    IPAddress localIP() { return _localIP; }
    String macAddress();
    void macAddress(uint8_t* mac) { memcpy(mac, _mac, 6); }
    IPAddress networkID() { return {192, 168, 1, 0}; }
    void reconnect() { _connectCountdown = _connectMax; }
    int RSSI() { return 1; }
    bool setHostname(const char* name);
    String SSID() { return {_ssid}; }
    IPAddress subnetMask() const { return _subnetIP; }

    // testing
    void connectIn(int connectCount);
    void reset();
private:
    byte _mac[6]{};
    char _name[20] = {0};
    char _ssid[20] = {0};
    IPAddress _localIP;
    IPAddress _gatewayIP;
    IPAddress _subnetIP;
    IPAddress _primaryDNSIP;
    IPAddress _secondaryDNSIP;
    int _connectMax = 5;
    int _connectCountdown = _connectMax;
};

#define WIFI_STA 1

extern WiFiClass WiFi;

#endif
