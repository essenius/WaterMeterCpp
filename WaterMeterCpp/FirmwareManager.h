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

// Since the device won't be in a place that's easily reachable with a USB cable, we let it set itself via OTA.
// We do this with a pull mechanism when it reboots. 
// It looks for a specified url: https://base-url/path/device-name.version which contains available build version.
// If that number is higher than the build of the device, it updates itself from https://base-url/path/device-name.bin
// I got the inspiration for this mechanism from https://www.bakke.online/index.php/2017/06/02/self-updating-ota-firmware-for-esp8266/

#ifndef HEADER_FIRMWAREMANAGER
#define HEADER_FIRMWAREMANAGER

#ifdef ESP32
#include <WiFiClient.h>
#else
#include "NetMock.h"
#endif
#include "Configuration.h"
#include "EventClient.h"

class FirmwareManager : public EventClient {
public:
    explicit FirmwareManager(EventServer* eventServer, const FirmwareConfig* firmwareConfig, const char* buildVersion);
    void begin(WiFiClient* client, const char* machineId);
    void loadUpdate() const;
    void tryUpdate();
    bool updateAvailable() const;
private:
    static constexpr int BASE_URL_SIZE = 100;
    static constexpr const char* IMAGE_EXTENSION = ".bin";
    static constexpr const char* VERSION_EXTENSION = ".version";
    WiFiClient* _client = nullptr;
    const char* _buildVersion;
    bool _justRebooted = true;
    char _machineId[20] = {};
    const FirmwareConfig* _firmwareConfig;
};
#endif
