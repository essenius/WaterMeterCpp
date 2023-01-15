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

#include "SafeCString.h"
#include "Configuration.h"
//#include "secrets.h"

Configuration::Configuration(Preferences* preferences) : _preferences(preferences) {}

constexpr const char* IP = "ip";

constexpr const char* LOCAL = "local";
constexpr const char* GATEWAY = "gateway";
constexpr const char* SUBNET_MASK = "subNetMask";
constexpr const char* DNS1 = "dns1";
constexpr const char* DNS2 = "dns2";

constexpr const char* MQTT = "mqtt";
constexpr const char* BROKER = "broker";
constexpr const char* PORT = "port";
constexpr const char* USER = "user";
constexpr const char* PASSWORD = "password";
constexpr const char* USE_TLS = "useTls";

constexpr const char* WIFI = "wifi";
constexpr const char* DEVICE_NAME = "deviceName";
constexpr const char* SSID = "ssid";
constexpr const char* BSSID = "bssid";

constexpr const char* TLS = "tls";
constexpr const char* ROOT_CA_CERT = "rootCaCert";
constexpr const char* DEVICE_CERT = "deviceCert";
constexpr const char* DEVICE_KEY = "deviceKey";

constexpr const char* FIRMWARE = "firmware";
constexpr const char* URL = "url";

void Configuration::begin(const bool useSecrets) {
    // we use both an ifdef and an if to enable testing without having to recompile
#ifdef HEADER_SECRETS
    if (useSecrets) {
        putIpConfig(&CONFIG_IP);
        putMqttConfig(&CONFIG_MQTT);
        putTlsConfig(&CONFIG_TLS);
        putWifiConfig(&CONFIG_WIFI);
        putFirmwareConfig(&CONFIG_FIRMWARE);
    }
#endif

    getIpConfig();
    _next = getWifiConfig(_buffer);
    _next = getTlsConfig(_next);
    _next = getMqttConfig(_next);
    getFirmwareConfig(_next);
}

char* Configuration::getFirmwareConfig(char* start) {
    _preferences->begin(FIRMWARE, true);
    firmware.baseUrl = storeToBuffer(URL, &start);
    _preferences->end();
    return start;
}

void Configuration::getIpConfig() {
    _preferences->begin(IP, true);
    ip.localIp = _preferences->getUInt(LOCAL, 0);
    ip.gateway = _preferences->getUInt(GATEWAY, 0);
    ip.subnetMask = _preferences->getUInt(SUBNET_MASK, 0);
    ip.primaryDns = _preferences->getUInt(DNS1, 0);
    ip.secondaryDns = _preferences->getUInt(DNS2, 0);
    _preferences->end();
}

char* Configuration::getMqttConfig(char* start) {
    _preferences->begin(MQTT, true);
    mqtt.broker = storeToBuffer(BROKER, &start);
    mqtt.port = _preferences->getUInt(PORT, 1883);
    mqtt.user = storeToBuffer(USER, &start);
    mqtt.password = storeToBuffer(PASSWORD, &start);
    mqtt.useTls = _preferences->getBool(USE_TLS, mqtt.port != 1883);
    _preferences->end();
    return start;
}

char* Configuration::getTlsConfig(char* start) {
    _preferences->begin(TLS, true);
    tls.rootCaCertificate = storeToBuffer(ROOT_CA_CERT, &start);
    tls.deviceCertificate = storeToBuffer(DEVICE_CERT, &start);
    tls.devicePrivateKey = storeToBuffer(DEVICE_KEY, &start);
    _preferences->end();
    return start;
}

char* Configuration::getWifiConfig(char* start) {
    _preferences->begin(WIFI, true);
    wifi.deviceName = storeToBuffer(DEVICE_NAME, &start);
    wifi.ssid = storeToBuffer(SSID, &start);
    wifi.password = storeToBuffer(PASSWORD, &start);
    if (_preferences->isKey(BSSID)) {
        _preferences->getBytes(BSSID, start, 6);
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
    _preferences->begin(FIRMWARE, false);
    _preferences->putString(URL, firnwareConfig->baseUrl);
    _preferences->end();
}

void Configuration::putIpConfig(const IpConfig* ipConfig) const {
    if (ipConfig == nullptr) return;
    _preferences->begin(IP, false);
    _preferences->clear();
    _preferences->putUInt(LOCAL, ipConfig->localIp);
    _preferences->putUInt(GATEWAY, ipConfig->gateway);
    _preferences->putUInt(SUBNET_MASK, ipConfig->subnetMask);
    _preferences->putUInt(DNS1, ipConfig->primaryDns);
    _preferences->putUInt(DNS2, ipConfig->secondaryDns);
    _preferences->end();
}

void Configuration::putMqttConfig(const MqttConfig* mqttConfig) const {
    if (mqttConfig == nullptr) return;
    _preferences->begin(MQTT, false);
    _preferences->clear();
    putStringIfNotNull(BROKER, mqttConfig->broker);
    if (mqttConfig->port != 0) {
        _preferences->putUInt(PORT, mqttConfig->port);
    }
    putStringIfNotNull(USER, mqttConfig->user);
    putStringIfNotNull(PASSWORD, mqttConfig->password);
    _preferences->putBool(USE_TLS, mqttConfig->useTls);
    _preferences->end();
}

void Configuration::putStringIfNotNull(const char* key, const char* value) const {
    if (value != nullptr) {
        _preferences->putString(key, value);
    }
}

void Configuration::putTlsConfig(const TlsConfig* tlsConfig) const {
    if (tlsConfig == nullptr) return;
    _preferences->begin(TLS, false);
    _preferences->clear();
    putStringIfNotNull(ROOT_CA_CERT, tlsConfig->rootCaCertificate);
    putStringIfNotNull(DEVICE_CERT, tlsConfig->deviceCertificate);
    putStringIfNotNull(DEVICE_KEY, tlsConfig->devicePrivateKey);
    _preferences->end();
}

void Configuration::putWifiConfig(const WifiConfig* wifiConfig) const {
    if (wifiConfig == nullptr) return;
    _preferences->begin(WIFI, false);
    _preferences->clear();
    putStringIfNotNull(DEVICE_NAME, wifiConfig->deviceName);
    putStringIfNotNull(SSID, wifiConfig->ssid);
    putStringIfNotNull(PASSWORD, wifiConfig->password);
    if (wifiConfig->bssid != nullptr) {
        _preferences->putBytes(BSSID, wifiConfig->bssid, 6);
    }
    _preferences->end();
}

int Configuration::freeBufferSpace() const {
    return BUFFER_SIZE - static_cast<int>(_next - _buffer);
}

char* Configuration::storeToBuffer(const char* key, char** startLocation) {
    if (_preferences->isKey(key)) {
        safePointerStrcpy(*startLocation, _buffer, _preferences->getString(key).c_str());
        const auto returnValue = *startLocation;
        *startLocation += strlen(*startLocation) + 1;
        return returnValue;
    }
    return nullptr;
}
