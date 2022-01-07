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

#ifndef HEADER_CONNECTION
#define HEADER_CONNECTION
#include "MqttGateway.h"
#include "Wifi.h"

constexpr unsigned long SECONDS = 1000UL * 1000UL;
constexpr unsigned long WIFI_INITIAL_WAIT_DURATION = 20UL * SECONDS;
constexpr unsigned long WIFI_RECONNECT_WAIT_DURATION = 10UL * SECONDS;
constexpr unsigned int MAX_RECONNECT_FAILURES = 5;
constexpr unsigned long MQTT_RECONNECT_WAIT_DURATION = 2UL * SECONDS;

enum class ConnectionState {
    Init = 0, Disconnected,
    WifiConnecting, WifiConnected, WifiReady,
    MqttConnecting, WaitingForMqttReconnect, MqttConnected, MqttReady
};

class Connection
{
public:
    Connection(EventServer* eventServer, Wifi* wifi, MqttGateway* mqttGatway);
    void begin();
    ConnectionState connectWifi();
    ConnectionState connectMqtt();
private:
    EventServer* _eventServer;
    ConnectionState _state;
    unsigned long _wifiConnectTimestamp = 0L;
    unsigned long _mqttConnectTimestamp = 0L;

    Wifi* _wifi;
    MqttGateway* _mqttGateway;
    unsigned long _waitDuration = WIFI_INITIAL_WAIT_DURATION;
    unsigned int _wifiConnectionFailureCount = 0;

    void handleInit();
    void handleWifiConnecting();
    void handleWifiConnected();
    void handleWifiReadyForSetup();
    void handleWifiReady();
    void handleMqttConnecting();
    void handleWaitingForMqttReconnect();
    void handleMqttConnected();
    void handleMqttReady();
    void handleDisconnected();

};

#endif
