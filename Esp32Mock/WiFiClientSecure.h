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

// Mock implementation of WiFiClientSecure for unit testing (not targeting the ESP32)

// Disabling warnings caused by mimicking existing interface
// ReSharper disable CppInconsistentNaming
// ReSharper disable CppMemberFunctionMayBeStatic
// ReSharper disable CppParameterNeverUsed

#ifndef HEADER_WIFICLIENTSECURE
#define HEADER_WIFICLIENTSECURE

#include "WiFiClient.h"

class WiFiClientSecure final : public WiFiClient {
public:
    void setCACert(const char* cert) { }
    void setCertificate(const char* cert) { }
    void setPrivateKey(const char* cert) { }
    void setInsecure() { _insecure = true; }

    // testing
    bool isSecure() const { return !_insecure; }
    const char* getType() override { return "WifiClientSecure"; }
private:
    bool _insecure = false;
};

#endif
