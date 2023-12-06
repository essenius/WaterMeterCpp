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

#include <SafeCString.h>
#include "Configuration.h"
#include "secrets.h"

Configuration::Configuration(Preferences* preferences) : _preferences(preferences) {}

constexpr const char* Ip = "ip";

constexpr const char* Local = "local";
constexpr const char* Gateway = "gateway";
constexpr const char* SubnetMask = "subNetMask";
constexpr const char* Dns1 = "dns1";
constexpr const char* Dns2 = "dns2";

constexpr const char* Mqtt = "mqtt";
constexpr const char* Broker = "broker";
constexpr const char* Port = "port";
constexpr const char* User = "user";
constexpr const char* Password = "password";
constexpr const char* UseTls = "useTls";

constexpr const char* Wifi = "wifi";
constexpr const char* DeviceName = "deviceName";
constexpr const char* Ssid = "ssid";
constexpr const char* Bssid = "bssid";

constexpr const char* Tls = "tls";
constexpr const char* RootCaCert = "rootCaCert";
constexpr const char* DeviceCert = "deviceCert";
constexpr const char* DeviceKey = "deviceKey";

constexpr const char* Firmware = "firmware";
constexpr const char* Url = "url";

void Configuration::begin(const bool useSecrets) {
    // we use both an ifdef and an if to enable testing without having to recompile
#ifdef HEADER_SECRETS
    if (useSecrets) {
        putIpConfig(&configIp);
        putMqttConfig(&ConfigMqtt);
        putTlsConfig(&ConfigTls);
        putWifiConfig(&ConfigWifi);
        putFirmwareConfig(&ConfigFirmware);
    }
#endif

    getIpConfig();
    _next = getWifiConfig(_buffer);
    _next = getTlsConfig(_next);
    _next = getMqttConfig(_next);
    getFirmwareConfig(_next);
}

int Configuration::freeBufferSpace() const {
    return BufferSize - static_cast<int>(_next - _buffer);
}

char* Configuration::getFirmwareConfig(char* start) {
    _preferences->begin(Firmware, true);
    firmware.baseUrl = storeToBuffer(Url, &start);
    _preferences->end();
    return start;
}

void Configuration::getIpConfig() {
    _preferences->begin(Ip, true);
    ip.localIp = _preferences->getUInt(Local, 0);
    ip.gateway = _preferences->getUInt(Gateway, 0);
    ip.subnetMask = _preferences->getUInt(SubnetMask, 0);
    ip.primaryDns = _preferences->getUInt(Dns1, 0);
    ip.secondaryDns = _preferences->getUInt(Dns2, 0);
    _preferences->end();
}

char* Configuration::getMqttConfig(char* start) {
    _preferences->begin(Mqtt, true);
    mqtt.broker = storeToBuffer(Broker, &start);
    mqtt.port = _preferences->getUInt(Port, 1883);
    mqtt.user = storeToBuffer(User, &start);
    mqtt.password = storeToBuffer(Password, &start);
    mqtt.useTls = _preferences->getBool(UseTls, mqtt.port != 1883);
    _preferences->end();
    return start;
}

char* Configuration::getTlsConfig(char* start) {
    _preferences->begin(Tls, true);
    tls.rootCaCertificate = storeToBuffer(RootCaCert, &start);
    tls.deviceCertificate = storeToBuffer(DeviceCert, &start);
    tls.devicePrivateKey = storeToBuffer(DeviceKey, &start);
    _preferences->end();
    return start;
}

char* Configuration::getWifiConfig(char* start) {
    _preferences->begin(Wifi, true);
    wifi.deviceName = storeToBuffer(DeviceName, &start);
    wifi.ssid = storeToBuffer(Ssid, &start);
    wifi.password = storeToBuffer(Password, &start);
    if (_preferences->isKey(Bssid)) {
        _preferences->getBytes(Bssid, start, 6);
        wifi.bssid = reinterpret_cast<uint8_t*>(start);
        start += 6;
    }
    else {
        wifi.bssid = nullptr;
    }
    _preferences->end();
    return start;
}

void Configuration::putFirmwareConfig(const FirmwareConfig* firnwareConfig) const {
    if (firnwareConfig == nullptr) return;
    _preferences->begin(Firmware, false);
    _preferences->putString(Url, firnwareConfig->baseUrl);
    _preferences->end();
}

void Configuration::putIpConfig(const IpConfig* ipConfig) const {
    if (ipConfig == nullptr) return;
    _preferences->begin(Ip, false);
    _preferences->clear();
    _preferences->putUInt(Local, ipConfig->localIp);
    _preferences->putUInt(Gateway, ipConfig->gateway);
    _preferences->putUInt(SubnetMask, ipConfig->subnetMask);
    _preferences->putUInt(Dns1, ipConfig->primaryDns);
    _preferences->putUInt(Dns2, ipConfig->secondaryDns);
    _preferences->end();
}

void Configuration::putMqttConfig(const MqttConfig* mqttConfig) const {
    if (mqttConfig == nullptr) return;
    _preferences->begin(Mqtt, false);
    _preferences->clear();
    putStringIfNotNull(Broker, mqttConfig->broker);
    if (mqttConfig->port != 0) {
        _preferences->putUInt(Port, mqttConfig->port);
    }
    putStringIfNotNull(User, mqttConfig->user);
    putStringIfNotNull(Password, mqttConfig->password);
    _preferences->putBool(UseTls, mqttConfig->useTls);
    _preferences->end();
}

void Configuration::putStringIfNotNull(const char* key, const char* value) const {
    if (value != nullptr) {
        _preferences->putString(key, value);
    }
}

void Configuration::putTlsConfig(const TlsConfig* tlsConfig) const {
    if (tlsConfig == nullptr) return;
    _preferences->begin(Tls, false);
    _preferences->clear();
    putStringIfNotNull(RootCaCert, tlsConfig->rootCaCertificate);
    putStringIfNotNull(DeviceCert, tlsConfig->deviceCertificate);
    putStringIfNotNull(DeviceKey, tlsConfig->devicePrivateKey);
    _preferences->end();
}

void Configuration::putWifiConfig(const WifiConfig* wifiConfig) const {
    if (wifiConfig == nullptr) return;
    _preferences->begin(Wifi, false);
    _preferences->clear();
    putStringIfNotNull(DeviceName, wifiConfig->deviceName);
    putStringIfNotNull(Ssid, wifiConfig->ssid);
    putStringIfNotNull(Password, wifiConfig->password);
    if (wifiConfig->bssid != nullptr) {
        _preferences->putBytes(Bssid, wifiConfig->bssid, 6);
    }
    _preferences->end();
}

char* Configuration::storeToBuffer(const char* key, char** startLocation) {
    if (_preferences->isKey(key)) {
        SafeCString::pointerStrcpy(*startLocation, _buffer, _preferences->getString(key).c_str());
        const auto returnValue = *startLocation;
        *startLocation += strlen(*startLocation) + 1;
        return returnValue;
    }
    return nullptr;
}
