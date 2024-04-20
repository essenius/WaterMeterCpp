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

#pragma once

#include "WiFiManager.h"

namespace WaterMeterCppTest {
    using WaterMeter::EventServer;
    using WaterMeter::PayloadBuilder;
    using WaterMeter::WifiConfig;
    using WaterMeter::WiFiManager;

    class WiFiMock final : public WiFiManager {
    public:
        explicit WiFiMock(EventServer* eventServer, PayloadBuilder* payloadBuilder);
        void begin() override {}
        void disconnect() override {}
        bool isConnected() override { return _isConnected; }
        void reconnect() override {}
        bool needsReInit() override { return _needsReconnect; }
        void announceReady() override {}

        void setIsConnected(const bool connected) { _isConnected = connected; }
        void setNeedsReconnect(const bool needsReconnect) { _needsReconnect = needsReconnect; }

    private:
        bool _isConnected = false;
        bool _needsReconnect = true;
        static constexpr WifiConfig Config{"ssid", "password", nullptr, nullptr};
    };
}
