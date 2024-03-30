// Copyright 2022-2024 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

// Creates a Wi-Fi client. This allows for hiding specifics like using HTTPS or HTTP, and using certificates.

#ifndef HEADER_WIFI_CLIENT_FACTORY
#define HEADER_WIFI_CLIENT_FACTORY

#include <WiFiClientSecure.h>
#include "Configuration.h"

namespace WaterMeter {
    class WiFiClientFactory {
    public:
        explicit WiFiClientFactory(const TlsConfig* config);
        WiFiClient* create(bool useTls) const;
        WiFiClient* create(const char* url) const;
    private:
        const TlsConfig* _config;
    };
}
#endif
