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

#ifndef ESP32

#include "NetMock.h"

WiFiClass::WiFiClass() {
    reset();
    strcpy(_name, "esp32_001122334455");
    for (int i = 0; i < 6; i++) {
        _mac[i] =  i * 17;
    }
}

int HTTPClient::ReturnValue = 400;
int HTTPUpdate::ReturnValue = HTTP_UPDATE_NO_UPDATES;

HTTPUpdate httpUpdate;

WiFiClass WiFi;

IPAddress::IPAddress() {
    //_value[0] = 0;
}

IPAddress::IPAddress(uint8_t oct1, uint8_t oct2, uint8_t oct3, uint8_t oct4) {
    _address.bytes[0] = oct1;
    _address.bytes[1] = oct2;
    _address.bytes[2] = oct3;
    _address.bytes[3] = oct4;
}

IPAddress::IPAddress(const uint8_t* address) {
    memcpy(_address.bytes, address, sizeof(_address.bytes));
}

IPAddress& IPAddress::operator=(uint32_t address)
{
    _address.dword = address;
    return *this;
}

IPAddress& IPAddress::operator=(const uint8_t* address)
{
    memcpy(_address.bytes, address, sizeof(_address.bytes));
    return *this;
}

String IPAddress::toString() const
{
    char buffer[16];
    sprintf(buffer, "%u.%u.%u.%u", _address.bytes[0], _address.bytes[1], _address.bytes[2], _address.bytes[3]);
    return String(buffer);
}


String WiFiClass::macAddress() {
    char buffer[20];
    sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", _mac[0], _mac[1], _mac[2], _mac[3], _mac[4], _mac[5]);
    return {buffer};
}

void WiFiClass::reset() {
    _localIP = IPAddress(10, 0, 0, 2);
    _gatewayIP = IPAddress(10, 0, 0, 1);
    _subnetIP = IPAddress(255, 255, 0, 0);
    _primaryDNSIP = IPAddress(8, 8, 8, 8);
    _secondaryDNSIP = IPAddress(8, 8, 4, 4);
}

const IPAddress NO_IP = IPAddress(0, 0, 0, 0);

bool WiFiClass::config(IPAddress local, IPAddress gateway, IPAddress subnet, IPAddress primaryDNS, IPAddress secondaryDNS) {
    _localIP = local;
    _gatewayIP = gateway;
    _subnetIP = subnet;
    _primaryDNSIP = primaryDNS == NO_IP ? _gatewayIP : primaryDNS;
    _secondaryDNSIP = secondaryDNS == NO_IP ? _primaryDNSIP : secondaryDNS;
     return true; 
}

bool WiFiClass::isConnected() {
    _connectCountdown--;
    if (_connectCountdown == 0) {
        _connectCountdown = 10;
        return true;
    }
    return false;
}

bool WiFiClass::setHostname(const char* name) { 
    strcpy(_name, name); 
    return strlen(name) > 0;
}

#endif
