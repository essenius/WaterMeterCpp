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


// The states that the connection state machine can have

#ifndef HEADER_CONNECTION_STATE
#define HEADER_CONNECTION_STATE

namespace WaterMeter {

    enum class ConnectionState: uint8_t {
        Init = 0,
        Disconnected,
        WifiConnecting,
        WifiConnected,
        WifiReady,
        MqttConnecting,
        WaitingForMqttReconnect,
        MqttConnected,
        MqttReady,
        RequestTime,
        SettingTime,
        CheckFirmware
    };
}
#endif
