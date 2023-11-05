// Copyright 2022-2023 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

// Connector runs a process that connects with the outside world. The connect method uses a getState machine
// to make the connection with Wifi, get the time, check for a firmware upgrade and connect to the MQTT server

#ifndef HEADER_CONNECTION
#define HEADER_CONNECTION
#include "MqttGateway.h"
#include "WiFiManager.h"
#include "FirmwareManager.h"
#include "ChangePublisher.h"
#include "TimeServer.h"
#include "ConnectionState.h"
#include "DataQueue.h"
#include "QueueClient.h"
#include "Serializer.h"

constexpr unsigned int MaxReconnectFailures = 5;
constexpr unsigned long Seconds = 1000UL * 1000UL;
constexpr unsigned long MqttReconnectWaitDuration = 2UL * Seconds;
constexpr unsigned long TimeserverWaitDuration = 10UL * Seconds;
constexpr unsigned long WifiInitialWaitDuration = 20UL * Seconds;
constexpr unsigned long WifiReconnectWaitDuration = 10UL * Seconds;

class Connector final : public EventClient {
public:
    Connector(EventServer* eventServer, WiFiManager* wifi, MqttGateway* mqttGateway, TimeServer* timeServer,
              FirmwareManager* firmwareManager, DataQueue* samplerDataQueue, DataQueue* communicatorDataQueue,
              Serializer* serializer, QueueClient* samplerQueueClient, QueueClient* communicatorQueueClient);
    void begin(const Configuration* configuration);
    ConnectionState connect();
    ConnectionState loop();
    static void task(void* parameter);

private:
    unsigned long _wifiConnectTimestamp = 0UL;
    unsigned long _mqttConnectTimestamp = 0UL;
    unsigned long _requestTimeTimestamp = 0UL;
    unsigned long _lastAnnouncementTimestampMillis = 0UL;

    WiFiManager* _wifi;
    MqttGateway* _mqttGateway;
    TimeServer* _timeServer;
    FirmwareManager* _firmwareManager;
    DataQueue* _samplerDataQueue;
    DataQueue* _communicatorDataQueue;
    Serializer* _serializer;
    QueueClient* _samplerQueueClient;
    QueueClient* _communicatorQueueClient;
    ChangePublisher<ConnectionState> _state;
    unsigned long _waitDuration = WifiInitialWaitDuration;
    unsigned int _wifiConnectionFailureCount = 0;

    void handleCheckFirmware();
    void handleDisconnected();
    void handleInit();
    void handleMqttConnected();
    void handleMqttConnecting();
    void handleMqttReady();
    void handleRequestTime();
    void handleSettingTime();
    void handleWaitingForMqttReconnect();
    void handleWifiConnected();
    void handleWifiConnecting();
    void handleWifiReady();
};

#endif
