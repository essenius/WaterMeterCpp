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
#include <ESP.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClient.h>
#endif

#include "FirmwareManager.h"
#include "EventServer.h"
#include "SafeCString.h"

FirmwareManager::FirmwareManager(EventServer* eventServer, const char* baseUrl, const char* buildVersion) :
    EventClient(eventServer), _buildVersion(buildVersion) {
    safeStrcpy(_baseUrl, baseUrl);
}

void FirmwareManager::begin(WiFiClient* client, const char* machineId) {
    _client = client;
    safeStrcpy(_machineId, machineId);
}

bool FirmwareManager::updateAvailable() const {
    char versionUrl[BASE_URL_SIZE];
    safeStrcpy(versionUrl, _baseUrl);
    safeStrcat(versionUrl, _machineId);
    safeStrcat(versionUrl, VERSION_EXTENSION);
    HTTPClient httpClient;
    httpClient.begin(*_client, versionUrl);
    bool newBuildAvailable = false;
    char buffer[255];

    const int httpCode = httpClient.GET();
    if (httpCode == 200) {
        String newVersion = httpClient.getString();
        newBuildAvailable = strcmp(newVersion.c_str(), _buildVersion) != 0;
        if (newBuildAvailable) {
            safeSprintf(buffer, "Current firmware version: '%s'; available version: '%s'\n", _buildVersion, newVersion.c_str());
            _eventServer->publish(Topic::Info, buffer);
        }
    }
    else {
        safeSprintf(buffer, "Firmware version check to '%s' failed with response code %d\n", versionUrl, httpCode);
        _eventServer->publish(Topic::CommunicationError, buffer);
    }
    httpClient.end();
    return newBuildAvailable;
}

void FirmwareManager::loadUpdate() const {
    char buffer[BASE_URL_SIZE];
    safeStrcpy(buffer, _baseUrl);
    safeStrcat(buffer, IMAGE_EXTENSION);

    // This should normally result in a reboot.
    const t_httpUpdate_return returnValue = httpUpdate.update(*_client, buffer);

    if (returnValue == HTTP_UPDATE_FAILED) {
        safeSprintf(
            buffer, 
            "Firmware update failed (%d): %s\n", 
            httpUpdate.getLastError(),
            httpUpdate.getLastErrorString().c_str());
        _eventServer->publish(Topic::CommunicationError, buffer);
        return;
    }
    safeSprintf(
        buffer, 
        "Firmware not updated (%d/%d): %s\n", 
        returnValue, 
        httpUpdate.getLastError(),
        httpUpdate.getLastErrorString().c_str());
    _eventServer->publish(Topic::Info, buffer);
}

void FirmwareManager::tryUpdate() {
    // Make sure this is only done just after rebooting. We don't want reboots in the middle of a flow.
    if (_justRebooted && updateAvailable()) {
        loadUpdate();
    }
    _justRebooted = false;
}
