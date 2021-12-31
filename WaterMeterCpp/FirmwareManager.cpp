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

#ifdef ESP32
#include <ESP.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClient.h>
#else
#include "ArduinoMock.h"
#endif

#include <string.h>
#include "FirmwareManager.h" 
#include "EventServer.h"

FirmwareManager::FirmwareManager(EventServer* eventServer) : EventClient("FirmwareManager", eventServer)
{}

void FirmwareManager::begin(WiFiClient* client, const char* baseUrl, const char* machineId) {
  _client = client;
  strcpy(_baseUrl, baseUrl);
  strcat(_baseUrl, machineId);
}

bool FirmwareManager::updateAvailableFor(const char* currentVersion) {
    char versionUrl[BASE_URL_SIZE];
    strcpy(versionUrl, _baseUrl);
    strcat(versionUrl, VERSION_EXTENSION);
    HTTPClient httpClient;
    httpClient.begin(*_client, versionUrl);
    bool newBuildAvailable = false;
    char buffer[100];

    int httpCode = httpClient.GET();
    if (httpCode == 200) {
        const char* newVersion = httpClient.getString().c_str();
        newBuildAvailable = strcmp(newVersion, currentVersion) !=0;
        if (newBuildAvailable) {
            sprintf(buffer, "Current firmware version: %s; available version: %s\n", currentVersion, newVersion);
            _eventServer->publish(Topic::Info, buffer);
        }
    } else {
        sprintf(buffer,"Firmware version check failed with response code %d\n", httpCode);
        _eventServer->publish(Topic::Error, buffer);
  }
  httpClient.end();
  return newBuildAvailable;
}

void FirmwareManager::update() {
    char buffer[BASE_URL_SIZE];
    strcpy(buffer, _baseUrl);
    strcat(buffer, IMAGE_EXTENSION);

    // This should normally result in a reboot.
    t_httpUpdate_return returnValue = httpUpdate.update(*_client, buffer);

    if (returnValue == HTTP_UPDATE_FAILED) {
        sprintf(buffer, "Firmware update failed (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
        _eventServer->publish(Topic::Error, buffer);
        return;
    }
    sprintf(buffer, "Firmware not updated (%d/%d): %s\n", returnValue, httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
    _eventServer->publish(Topic::Info, buffer);
}

void FirmwareManager::tryUpdateFrom(const char* currentVersion) {
    if (updateAvailableFor(currentVersion)) {
        update();
    }
}
