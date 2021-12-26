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

#ifndef HEADER_WIFI_H
#define HEADER_WIFI_H

#include "PayloadBuilder.h"

#ifdef ESP32
#include <WiFi.h>
#include <WiFiClientSecure.h>
#else
#include "NetMock.h"
#endif

class Wifi {

public:
    void begin();
    const char* getHostName();
    bool isConnected();
    const char* macAddress();
    const char* statusSummary();
    WiFiClientSecure* getClient();
private:
    static const int HOSTNAME_LENGTH = 30;
    char _hostName[HOSTNAME_LENGTH] = { 0 };
    PayloadBuilder _payloadBuilder;
    WiFiClientSecure _wifiClient;
    static const int MAC_ADDRESS_SIZE = 14;
    char _macAddress[MAC_ADDRESS_SIZE];
};
#endif
