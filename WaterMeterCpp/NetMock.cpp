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

#ifndef ESP32

#include "NetMock.h"

WiFiClass::WiFiClass() { 
    strcpy(_name, "");
    for (int i = 0; i < 6; i++) {
        _mac[i] = (5 - i) * 17; 
    }
}

int HTTPClient::ReturnValue = 400;
int HTTPUpdate::ReturnValue = HTTP_UPDATE_NO_UPDATES;

HTTPUpdate httpUpdate;

WiFiClass WiFi;

IPAddress::IPAddress(uint8_t oct1, uint8_t oct2, uint8_t oct3, uint8_t oct4) {
    sprintf(_value, "%d.%d.%d.%d", oct1, oct2, oct3, oct4);
}

String WiFiClass::macAddress() {
    char buffer[20];
    sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", _mac[5], _mac[4], _mac[3], _mac[2], _mac[1], _mac[0]);
    return String(buffer);
}

bool WiFiClass::setHostname(const char* name) { 
    strcat(_name, name); 
    return true; 
}

#endif
