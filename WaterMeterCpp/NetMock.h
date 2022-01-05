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

#ifndef ESP32

#ifndef HEADER_NETMOCK_H
#define HEADER_NETMOCK_H


#include "ArduinoMock.h"

class Client {};

class WiFiClient : public Client {
public:
    WiFiClient() = default;

    bool isConnected() {
        return true;
    }
};

class WiFiClientSecure : public WiFiClient {
public:
    void setCACert(const char* cert) {}
    void setCertificate(const char* cert) {}
    void setPrivateKey(const char* cert) {}
};

class String {
public:
    String(const char* value) {
        strcpy(_value, value);
    }

    int toInt() { return atoi(_value); }
    const char* c_str() { return _value; }

private:
    char _value[30];
};

class HTTPClient {
public:
    HTTPClient() = default;
    void end() {}
    bool begin(WiFiClient& client, const char* url) { return true; }
    int GET() { return ReturnValue; };
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
    String getLastErrorString() { return String("OK"); }
    static int ReturnValue;
};

extern HTTPUpdate httpUpdate;

class IPAddress {
public:
    IPAddress() = default;
    IPAddress(uint8_t oct1, uint8_t oct2, uint8_t oct3, uint8_t oct4);
    IPAddress(const uint8_t* address);
    IPAddress& operator=(uint32_t address);
    IPAddress& operator=(const uint8_t* address);
    String toString() const;

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
    void mode(int i) {}

    void begin(const char* ssid, const char* password, int ignore = 0, const uint8_t* _bssid = nullptr) {
        strcpy(_ssid, ssid);
    }

    bool config(IPAddress localIP, IPAddress gateway, IPAddress subnet, IPAddress dns1 = IPAddress(),
        IPAddress dns2 = IPAddress());
    bool isConnected();
    bool setHostname(const char* name);
    void reconnect() { _connectCountdown = 10; }
    const char* getHostname() { return _name; }
    String SSID() { return String(_ssid); }
    String macAddress();
    void macAddress(uint8_t* mac) { memcpy(mac, _mac, 6); }
    int RSSI() { return 1; }
    int channel() { return 13; }
    IPAddress networkID() { return IPAddress(192, 168, 1, 0); }
    IPAddress localIP() { return _localIP; }
    IPAddress gatewayIP() { return _gatewayIP; }
    IPAddress dnsIP(int i = 0) { return i == 0 ? _primaryDNSIP : _secondaryDNSIP; }
    IPAddress subnetMask() const { return _subnetIP; }
    String BSSIDstr() { return String("55:44:33:22:11:00"); }
    void disconnect() { _connectCountdown = 10; }
    // testing
    void reset();

private:
    char _name[20] = {0};
    char _ssid[20] = {0};
    byte _mac[6];
    IPAddress _localIP;
    IPAddress _gatewayIP;
    IPAddress _subnetIP;
    IPAddress _primaryDNSIP;
    IPAddress _secondaryDNSIP;
    int _connectCountdown = 10;
};

#define WIFI_STA 1

extern WiFiClass WiFi;


#endif

#endif
