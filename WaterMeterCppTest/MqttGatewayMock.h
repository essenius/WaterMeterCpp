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

#pragma once

#include "../WaterMeterCpp/MqttGateway.h"

namespace WaterMeterCppTest {
    using WaterMeter::EventServer;
    using WaterMeter::MqttConfig;
    using WaterMeter::MqttGateway;
    using WaterMeter::WiFiClientFactory;

    class MqttGatewayMock final : public MqttGateway {
    public:
        explicit MqttGatewayMock(EventServer* eventServer, PubSubClient* mqttClient, WiFiClientFactory* wifiClientFactory);
        void announceReady() override { }
        void begin(const char* clientName) override { }
        bool isConnected() override { return _connect; }
        bool publishNextAnnouncement() override { return _connect; }
        bool hasAnnouncement() override;

        void setIsConnected(const bool isConnected) {
            _connect = isConnected;
            _announceCounter = 0;
        }

    private:
        bool _connect = false;
        int _announceCounter = 0;
        constexpr static MqttConfig MqttConfig{"broker", 1883, "user", "password", false};
    };
}
