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

#ifdef ESP32
#include <HTTPClient.h>
#else
#include <cstring>
#endif

#include "Wifi.h"

#include "EventServer.h"

const IPAddress Wifi::NO_IP = IPAddress(0, 0, 0, 0);

Wifi::Wifi(EventServer* eventServer, const char* ssid, const char* password, const char* hostName, const uint8_t* bssid) :
    EventClient("Wifi", eventServer),  _ssid(ssid), _password(password), _bssid(bssid) {
    if (hostName == nullptr) {
        _hostName = nullptr;
    }
    else {
        strcpy(_hostNameBuffer, hostName);
        _hostName = _hostNameBuffer;
    }
    _macAddress[0] = 0;
}


WiFiClient* Wifi::getClient() { return &_wifiClient; }

void Wifi::configure(
    const IPAddress localIp, const IPAddress gatewayIp, const IPAddress subnetMaskIp, 
    const IPAddress dns1Ip, const IPAddress dns2Ip) {
    _localIp = localIp;
    if (gatewayIp == NO_IP && localIp != NO_IP) {
        _gatewayIp = localIp;
        _gatewayIp[3] = 1;
    }
    else {
        _gatewayIp = gatewayIp;
    }
    _subnetMaskIp = subnetMaskIp == NO_IP ? IPAddress(255, 255, 255, 0) : subnetMaskIp;

    bool result;
    if (dns1Ip == NO_IP) {
        result = WiFi.config(_localIp, _gatewayIp, _subnetMaskIp);
    }
    else {
        if (dns2Ip == NO_IP) {
            result = WiFi.config(_localIp, _gatewayIp, _subnetMaskIp, dns1Ip);
        }
        else {
            result = WiFi.config(_localIp, _gatewayIp, _subnetMaskIp, dns1Ip, dns2Ip);
        }
    }
    if (!result) {
        _eventServer->publish(Topic::Error, "Could not configure Wifi with static IP");
    }
}

// There is an issue on ESP32 with DHCP, timing out after 12032 seconds. Workaround is setting a fixed IP
// address so we don't need DHCP. So, if we still want a dynamic IP, we first connect without config to
// get valid addresses, and then we disconnect, and reconnect using the just obtained addresses, fixed.
bool Wifi::needsReinit() {
    if (!isConnected()) return false;
    _needsReconnect = _localIp == NO_IP;
    if (_needsReconnect) _localIp = WiFi.localIP();
    if (_gatewayIp == NO_IP) _gatewayIp = WiFi.gatewayIP();
    if (_subnetMaskIp == NO_IP) _subnetMaskIp = WiFi.subnetMask();
    if (_dns1Ip == NO_IP) _dns1Ip = WiFi.dnsIP(0);
    if (_dns2Ip == NO_IP) _dns2Ip = WiFi.dnsIP(1);
    return _needsReconnect;
}

void Wifi::begin() {
    init();
}

void Wifi::init() {
    // need to set the host name before setting the mode
    if (_hostName != nullptr && !WiFi.setHostname(_hostName)) {
        _eventServer->publish(Topic::Error, "Could not set host name");
    }
    strcpy(_hostNameBuffer, WiFi.getHostname());
    _hostName = _hostNameBuffer;

    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid, _password, 0, _bssid);
}

void Wifi::setCertificates(const char* rootCACertificate, const char* deviceCertificate, const char* devicePrivateKey) {
    _wifiClient.setCACert(rootCACertificate);
    _wifiClient.setCertificate(deviceCertificate);
    _wifiClient.setPrivateKey(devicePrivateKey);
}

void Wifi::announceReady() {
    _eventServer->publish(Topic::Info, statusSummary());
    _eventServer->provides(this, Topic::IpAddress);
    _eventServer->provides(this, Topic::MacFormatted);
    _eventServer->provides(this, Topic::MacRaw);
}

const char* Wifi::getHostName() const { return _hostName; }

const char* Wifi::get(const Topic topic, const char* defaultValue) {
    switch(topic) {
    case Topic::IpAddress: {
        strcpy(_ipAddress,WiFi.localIP().toString().c_str());
        return _ipAddress;
    }
    case Topic::MacRaw:
    case Topic::MacFormatted:
        uint8_t mac[6];
        WiFi.macAddress(mac);
        if (topic == Topic::MacRaw)
            sprintf(_macAddress, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        else
            sprintf(_macAddress, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return _macAddress;
    default:
        return defaultValue;
    }
}

void Wifi::reconnect() {
    WiFi.reconnect();
}

void Wifi::disconnect() {
    WiFi.disconnect();
}

const char* Wifi::statusSummary() {
    _payloadBuilder.initialize();
    _payloadBuilder.writeParam("ssid", WiFi.SSID().c_str());
    _payloadBuilder.writeParam("hostname", getHostName());
    _payloadBuilder.writeParam("mac-address", WiFi.macAddress().c_str());
    _payloadBuilder.writeParam("rssi-dbm", WiFi.RSSI());
    _payloadBuilder.writeParam("channel", WiFi.channel());
    _payloadBuilder.writeParam("network-id", WiFi.networkID().toString().c_str());
    _payloadBuilder.writeParam("ip-address", WiFi.localIP().toString().c_str());
    _payloadBuilder.writeParam("gateway-ip", WiFi.gatewayIP().toString().c_str());
    _payloadBuilder.writeParam("dns1-ip", WiFi.dnsIP(0).toString().c_str());
    _payloadBuilder.writeParam("dns2-ip", WiFi.dnsIP(1).toString().c_str());
    _payloadBuilder.writeParam("subnet-mask", WiFi.subnetMask().toString().c_str());
    _payloadBuilder.writeParam("bssid", WiFi.BSSIDstr().c_str());
    _payloadBuilder.writeGroupEnd();
    return _payloadBuilder.toString();
}

bool Wifi::isConnected() {
    return WiFi.isConnected();
}
