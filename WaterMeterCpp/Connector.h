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
#include "FirmwareManager.h"
#include "ChangePublisher.h"
#include "TimeServer.h"
#include "ConnectionState.h"
#include "DataQueue.h"
#include "QueueClient.h"

constexpr unsigned long SECONDS = 1000UL * 1000UL;
constexpr unsigned long WIFI_INITIAL_WAIT_DURATION = 20UL * SECONDS;
constexpr unsigned long WIFI_RECONNECT_WAIT_DURATION = 10UL * SECONDS;
constexpr unsigned long TIMESERVER_WAIT_DURATION = 10UL * SECONDS;
constexpr unsigned int MAX_RECONNECT_FAILURES = 5;
constexpr unsigned long MQTT_RECONNECT_WAIT_DURATION = 2UL * SECONDS;

class Connector: public EventClient
{
public:
    Connector(EventServer* eventServer, Wifi* wifi, MqttGateway* mqttGatway, TimeServer* timeServer, 
        FirmwareManager* firmwareManager, DataQueue* dataQueue, QueueClient* samplerQueueClient, QueueClient* communicatorQueueClient);
    void setup();
    static void task(void* parameter);

    ConnectionState loop();
    ConnectionState connect();
private:
    unsigned long _wifiConnectTimestamp = 0UL;
    unsigned long _mqttConnectTimestamp = 0UL;
    unsigned long _requestTimeTimestamp = 0UL;
    unsigned long _lastAnnouncementTimestampMillis = 0UL;

    Wifi* _wifi;
    MqttGateway* _mqttGateway;
    unsigned long _waitDuration = WIFI_INITIAL_WAIT_DURATION;
    unsigned int _wifiConnectionFailureCount = 0;
    TimeServer* _timeServer;
    FirmwareManager* _firmwareManager;
    DataQueue* _dataQueue;
    QueueClient* _samplerQueueClient;
    QueueClient* _communicatorQueueClient;
    ChangePublisher<ConnectionState> _state;

    void handleInit();
    void handleWifiConnecting();
    void handleWifiConnected();
    void handleRequestTime();
    void handleSettingTime();
    void handleWifiReady();
    void handleCheckFirmware();
    void handleMqttConnecting();
    void handleWaitingForMqttReconnect();
    void handleMqttConnected();
    void handleMqttReady();
    void handleDisconnected();

};

#endif
