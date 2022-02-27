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

// Mock implementation for unit testing (not targeting the ESP32)

// Disabling warnings caused by mimicking existing interfaces
// ReSharper disable CppInconsistentNaming
// ReSharper disable CppMemberFunctionMayBeStatic
// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppNonExplicitConversionOperator
// ReSharper disable CppParameterNeverUsed
// ReSharper disable CppParameterMayBeConst

#ifndef ESP32

#ifndef HEADER_NETMOCK_H
#define HEADER_NETMOCK_H

#include "ArduinoMock.h"
#include "SafeCString.h"
#include <string>

class Client {};

class WiFiClient : public Client {
public:
    WiFiClient() = default;
    bool isConnected() { return true; }
    void stop() {};
};

class WiFiClientSecure : public WiFiClient {
public:
    void setCACert(const char* cert) { }
    void setCertificate(const char* cert) { }
    void setPrivateKey(const char* cert) { }
};

class String {
public:
    String(const char* value) : _string(value) {

        //safeStrcpy(_value, value);
    }

    int toInt() { return std::stoi(_string); }
    const char* c_str() { return _string.c_str(); }

private:
    std::string _string;
    //char _value[30] {};
};

class HTTPClient {
public:
    HTTPClient() = default;

    bool begin(WiFiClient& client, const char* url) { return true; }
    void end() {}
    int GET() { return ReturnValue; }
    String getString() { return {"0.1.1"}; }
    static int ReturnValue;
};

constexpr int HTTP_UPDATE_FAILED = 0;
constexpr int HTTP_UPDATE_NO_UPDATES = 1;
constexpr int HTTP_UPDATE_OK = 2;

using t_httpUpdate_return = int;

class HTTPUpdate {
public:
    t_httpUpdate_return update(WiFiClient& client, const char* url) { return ReturnValue; }
    int getLastError() { return 0; }
    String getLastErrorString() { return {"OK"}; }
    static int ReturnValue;
};

extern HTTPUpdate httpUpdate;

class IPAddress {
public:
    IPAddress() = default;
    IPAddress(uint8_t oct1, uint8_t oct2, uint8_t oct3, uint8_t oct4);
    explicit IPAddress(const uint8_t* address);
    String toString() const;
    IPAddress& operator=(uint32_t address);
    IPAddress& operator=(const uint8_t* address);
    operator uint32_t() const { return _address.dword; }
    uint8_t operator[](int index) const { return _address.bytes[index]; }
    uint8_t& operator[](int index) { return _address.bytes[index]; }

private:
    union {
        uint8_t bytes[4];
        uint32_t dword;
    } _address{};

    uint8_t* raw_address() { return _address.bytes; }
};


class WiFiClass {
public:
    WiFiClass();

    void mode(int i) { }

    void begin(const char* ssid, const char* password, int ignore = 0, const uint8_t* _bssid = nullptr) {
        safeStrcpy(_ssid, ssid);
    }

    bool config(IPAddress local, IPAddress gateway, IPAddress subnet,
                IPAddress dns1 = IPAddress(), IPAddress dns2 = IPAddress());
    String BSSIDstr() { return { "55:44:33:22:11:00" }; }
    int channel() { return 13; }
    void disconnect() { _connectCountdown = _connectMax; }
    IPAddress dnsIP(int i = 0) { return i == 0 ? _primaryDNSIP : _secondaryDNSIP; }
    IPAddress gatewayIP() { return _gatewayIP; }
    const char* getHostname() { return _name; }
    bool isConnected();
    IPAddress localIP() { return _localIP; }
    String macAddress();
    void macAddress(uint8_t* mac) { memcpy(mac, _mac, 6); }
    IPAddress networkID() { return { 192, 168, 1, 0 }; }
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

#endif
