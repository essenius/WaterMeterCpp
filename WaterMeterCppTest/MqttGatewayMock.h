// Copyright 2022 Rik Essenius
// 
//   Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//   except in compliance with the License. You may obtain a copy of the License at
// 
//       http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software distributed under the License
//    is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and limitations under the License.

#pragma once
#include "../WaterMeterCpp/MqttGateway.h"

class MqttGatewayMock : public MqttGateway {
public:
    explicit MqttGatewayMock(EventServer* eventServer, PubSubClient* mqttClient);
    void announceReady() override { }
    void begin(Client* client, const char* clientName) override { }
    void connect() override { }
    bool isConnected() override { return _connect; }
    bool publishNextAnnouncement() override { return _connect; }
    bool hasAnnouncement() override;

    void setIsConnected(bool isConnected) {
        _connect = isConnected;
        _announceCounter = 0;
    }

private:
    bool _connect = false;
    int _announceCounter = 0;
    constexpr static MqttConfig _mqttConfig{ "broker", 1883, "user", "password" };
};