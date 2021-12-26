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

const char* VERSION_EXTENSION = ".version";
const char* IMAGE_EXTENSION = ".bin";

void FirmwareManager::begin(WiFiClient* client, const char* baseUrl, const char* machineId) {
  _client = client;
  strcpy(_baseUrl, baseUrl);
  strcat(_baseUrl, machineId);
}

bool FirmwareManager::updateAvailableFor(int currentVersion) {
  char versionUrl[BASE_URL_SIZE];
  strcpy(versionUrl, _baseUrl);
  strcat(versionUrl, VERSION_EXTENSION);
  Serial.printf("Firmware version URL: %s\n", versionUrl);

  HTTPClient httpClient;
  Serial.printf("Starting httpClient");

  httpClient.begin(*_client, versionUrl);
  Serial.printf("Started httpClient");
  bool returnValue = false;
  int httpCode = httpClient.GET();
  Serial.printf("Executed Get");
  if (httpCode == 200) {
    int newVersion = httpClient.getString().toInt();
    Serial.printf("Current firmware version: %d; available version: %d\n", currentVersion, newVersion);
    returnValue = newVersion > currentVersion;
  } else {
    Serial.printf("Firmware version check failed with response code %d\n", httpCode);
  }
  httpClient.end();
  return returnValue;
}

void FirmwareManager::update() {
  Serial.println("Updating firmware");
  char imageUrl[BASE_URL_SIZE];
  strcpy(imageUrl, _baseUrl);
  strcat(imageUrl, IMAGE_EXTENSION);

  t_httpUpdate_return returnValue = httpUpdate.update(*_client, imageUrl);

  switch(returnValue) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      break;  
  }
}

void FirmwareManager::tryUpdateFrom(int currentVersion) {
    if (updateAvailableFor(currentVersion)) {
    update();
  } else {
    Serial.println("No updates available");
  }
}
