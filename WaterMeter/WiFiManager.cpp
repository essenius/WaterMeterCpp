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

#include "WiFiManager.h"
#include <SafeCString.h>
#include "EventServer.h"
#include "WiFi.h"

namespace WaterMeter {
    WiFiManager::WiFiManager(EventServer* eventServer, const WifiConfig* wifiConfig, PayloadBuilder* payloadBuilder) :
        EventClient(eventServer), _payloadBuilder(payloadBuilder), _wifiConfig(wifiConfig) {}

    void WiFiManager::announceReady() {
        setStatusSummary();
        _eventServer->publish(this, Topic::WifiSummaryReady, true);
        _eventServer->provides(this, Topic::IpAddress);
        _eventServer->provides(this, Topic::MacFormatted);
        _eventServer->provides(this, Topic::MacRaw);
    }

    void WiFiManager::begin() {
        // need to set the host name before setting the mode
        if (_wifiConfig->deviceName == nullptr) {
            _hostName = nullptr;
        }
        else {
            SafeCString::strcpy(_hostNameBuffer, _wifiConfig->deviceName);
            _hostName = _hostNameBuffer;
        }
        _macAddress[0] = 0;
        if (_hostName != nullptr && !WiFi.setHostname(_hostName)) {
            _eventServer->publish(Topic::ConnectionError, "Could not set host name");
        }
        SafeCString::strcpy(_hostNameBuffer, WiFi.getHostname());
        _hostName = _hostNameBuffer;

        WiFi.mode(WIFI_STA);
        WiFi.begin(_wifiConfig->ssid, _wifiConfig->password, 0, _wifiConfig->bssid);
    }

    void WiFiManager::configure(const IpConfig* ipConfig) {
        _localIp = ipConfig->localIp;
        if (ipConfig->gateway == NoIp && ipConfig->localIp != NoIp) {
            _gatewayIp = ipConfig->localIp;
            _gatewayIp[3] = 1;
        }
        else {
            _gatewayIp = ipConfig->gateway;
        }
        _subnetMaskIp = ipConfig->subnetMask == NoIp ? IPAddress(255, 255, 255, 0) : ipConfig->subnetMask;

        bool result;
        if (ipConfig->primaryDns == NoIp) {
            result = WiFi.config(_localIp, _gatewayIp, _subnetMaskIp);
        }
        else {
            if (ipConfig->secondaryDns == NoIp) {
                result = WiFi.config(_localIp, _gatewayIp, _subnetMaskIp, ipConfig->primaryDns);
            }
            else {
                result = WiFi.config(_localIp, _gatewayIp, _subnetMaskIp, ipConfig->primaryDns, ipConfig->secondaryDns);
            }
        }
        if (!result) {
            _eventServer->publish(Topic::ConnectionError, "Could not configure Wifi with static IP");
        }
    }

    void WiFiManager::disconnect() {
        WiFi.disconnect();
    }

    const char* WiFiManager::get(const Topic topic, const char* defaultValue) {
        switch (topic) {
        case Topic::IpAddress: {
            SafeCString::strcpy(_ipAddress, WiFi.localIP().toString().c_str());
            return _ipAddress;
        }
        case Topic::MacRaw:
        case Topic::MacFormatted:
            uint8_t mac[6];
            WiFi.macAddress(mac);
            if (topic == Topic::MacRaw)
                SafeCString::sprintf(_macAddress, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            else
                SafeCString::sprintf(_macAddress, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            return _macAddress;
        default:
            return defaultValue;
        }
    }

    const char* WiFiManager::getHostName() const { return _hostName; }

    // There is an issue on ESP32 with DHCP, timing out after 12032 seconds. Workaround is setting a fixed IP
    // address so we don't need DHCP. So, if we still want a dynamic IP, we first connect without config to
    // get valid addresses, and then we disconnect, and reconnect using the just obtained addresses, fixed.
    bool WiFiManager::needsReinit() {
        if (!isConnected()) return false;
        _needsReconnect = _localIp == NoIp;
        if (_needsReconnect) _localIp = WiFi.localIP();
        if (_gatewayIp == NoIp) _gatewayIp = WiFi.gatewayIP();
        if (_subnetMaskIp == NoIp) _subnetMaskIp = WiFi.subnetMask();
        if (_dns1Ip == NoIp) _dns1Ip = WiFi.dnsIP(0);
        if (_dns2Ip == NoIp) _dns2Ip = WiFi.dnsIP(1);
        return _needsReconnect;
    }

    bool WiFiManager::isConnected() {
        return WiFi.isConnected();
    }

    void WiFiManager::reconnect() {
        WiFi.reconnect();
    }

    void WiFiManager::setStatusSummary() const {
        _payloadBuilder->initialize();
        _payloadBuilder->writeParam("ssid", WiFi.SSID().c_str());
        _payloadBuilder->writeParam("hostname", getHostName());
        _payloadBuilder->writeParam("mac-address", WiFi.macAddress().c_str());
        _payloadBuilder->writeParam("rssi-dbm", WiFi.RSSI());
        _payloadBuilder->writeParam("channel", WiFi.channel());
        _payloadBuilder->writeParam("network-id", WiFi.networkID().toString().c_str());
        _payloadBuilder->writeParam("ip-address", WiFi.localIP().toString().c_str());
        _payloadBuilder->writeParam("gateway-ip", WiFi.gatewayIP().toString().c_str());
        _payloadBuilder->writeParam("dns1-ip", WiFi.dnsIP(0).toString().c_str());
        _payloadBuilder->writeParam("dns2-ip", WiFi.dnsIP(1).toString().c_str());
        _payloadBuilder->writeParam("subnet-mask", WiFi.subnetMask().toString().c_str());
        _payloadBuilder->writeParam("bssid", WiFi.BSSIDstr().c_str());
        _payloadBuilder->writeGroupEnd();
    }
}