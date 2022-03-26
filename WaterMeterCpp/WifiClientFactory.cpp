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

#include "WiFiClientFactory.h"

WiFiClientFactory::WiFiClientFactory(const TlsConfig* config) : _config(config) {}

WiFiClient* WiFiClientFactory::create(const bool useTls) const {
    if (!useTls) return new WiFiClient();
    const auto client = new WiFiClientSecure();
    bool insecure = true;
    if (_config->rootCaCertificate != nullptr) {
        client->setCACert(_config->rootCaCertificate);
        insecure = false;
    }
    if (_config->deviceCertificate != nullptr) {
        client->setCertificate(_config->deviceCertificate);
        insecure = false;
    }
    if (_config->devicePrivateKey != nullptr) {
        client->setPrivateKey(_config->devicePrivateKey);
        insecure = false;
    }
    if (insecure) {
        client->setInsecure();
    }
    return client;
}

WiFiClient* WiFiClientFactory::create(const char* url) const {
    constexpr auto HTTPS = "https";
    return create(url != nullptr && strncmp(url, HTTPS, strlen(HTTPS)) == 0);
}
