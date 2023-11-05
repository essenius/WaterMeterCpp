// Copyright 2021-2022 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#include <HTTPClient.h>
#include <HTTPUpdate.h>

#include "FirmwareManager.h"
#include "EventServer.h"
#include "SafeCString.h"

FirmwareManager::FirmwareManager(
    EventServer* eventServer,
    const WiFiClientFactory* wifiClientFactory,
    const FirmwareConfig* firmwareConfig,
    const char* buildVersion) :

    EventClient(eventServer),
    _wifiClientFactory(wifiClientFactory),
    _buildVersion(buildVersion),
    _firmwareConfig(firmwareConfig) {}


void FirmwareManager::begin(const char* machineId) {
    safeStrcpy(_machineId, machineId);
}

void FirmwareManager::loadUpdate() const {
    char buffer[BaseUrlSize];
    safeStrcpy(buffer, _firmwareConfig->baseUrl);
    safeStrcat(buffer, _machineId);
    safeStrcat(buffer, ImageExtension);

    WiFiClient* updateClient = _wifiClientFactory->create(_firmwareConfig->baseUrl);

    httpUpdate.onProgress([=](const int current, const int total) {
        _eventServer->publish(Topic::UpdateProgress, current * 100 / total);
    });
    
    // This should normally result in a reboot.
    const t_httpUpdate_return returnValue = httpUpdate.update(*updateClient, buffer);
    if (returnValue == HTTP_UPDATE_FAILED) {
        safeSprintf(
            buffer,
            "Firmware update failed (%d): %s",
            httpUpdate.getLastError(),
            httpUpdate.getLastErrorString().c_str());
        _eventServer->publish(Topic::ConnectionError, buffer);
        return;
    }
    safeSprintf(
        buffer,
        "Firmware not updated (%d/%d): %s",
        returnValue,
        httpUpdate.getLastError(),
        httpUpdate.getLastErrorString().c_str());
    _eventServer->publish(Topic::Info, buffer);
    delete updateClient;
}

void FirmwareManager::tryUpdate() {
    // Make sure this is only done just after rebooting. We don't want reboots in the middle of a flow.
    if (_justRebooted && isUpdateAvailable()) {
        loadUpdate();
    }
    _justRebooted = false;
}

bool FirmwareManager::isUpdateAvailable() const {
    if (!_justRebooted) return false;
    char versionUrl[BaseUrlSize];
    safeStrcpy(versionUrl, _firmwareConfig->baseUrl);
    safeStrcat(versionUrl, _machineId);
    safeStrcat(versionUrl, VersionExtension);

    const auto client = _wifiClientFactory->create(_firmwareConfig->baseUrl);
    HTTPClient httpClient;
    httpClient.begin(*client, versionUrl);
    bool newBuildAvailable = false;
    char buffer[102]; // max size that the data queue can handle

    const int httpCode = httpClient.GET();
    if (httpCode == 200) {
        const String newVersion = httpClient.getString();
        newBuildAvailable = strcmp(newVersion.c_str(), _buildVersion) != 0;
        if (newBuildAvailable) {
            safeSprintf(buffer, "Current firmware: '%s'; available: '%s'", _buildVersion, newVersion.c_str());
            _eventServer->publish(Topic::Info, buffer);
        }
        else {
            safeSprintf(buffer, "Already on latest firmware: '%s'", _buildVersion);
            _eventServer->publish(Topic::Info, buffer);
        }
    }
    else {
        // This can be a long message, so separating out the URL
        safeSprintf(buffer, "Firmware version check failed with response code %d. URL:", httpCode);
        _eventServer->publish(Topic::ConnectionError, buffer);
        _eventServer->publish(Topic::Info, versionUrl);
    }
    // this disposes of client as well.
    httpClient.end();
    return newBuildAvailable;
}
