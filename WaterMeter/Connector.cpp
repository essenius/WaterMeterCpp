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

#include "Connector.h"
#include "LedDriver.h"
#include "QueueClient.h"

namespace WaterMeter {
    constexpr unsigned long OneHourInMillis = 3600UL * 1000UL;
    constexpr long LoopDelayMilliSeconds = 50;

    // TODO reduce number of parameters
    Connector::Connector(EventServer* eventServer, WiFiManager* wifi, MqttGateway* mqttGateway, TimeServer* timeServer,
        FirmwareManager* firmwareManager, DataQueue* samplerDataQueue, DataQueue* communicatorDataQueue,
        Serializer* serializer, QueueClient* samplerQueueClient, QueueClient* communicatorQueueClient) :
        EventClient(eventServer),
        _wifi(wifi),
        _mqttGateway(mqttGateway),
        _timeServer(timeServer),
        _firmwareManager(firmwareManager),
        _samplerDataQueue(samplerDataQueue),
        _communicatorDataQueue(communicatorDataQueue),
        _serializer(serializer),
        _samplerQueueClient(samplerQueueClient),
        _communicatorQueueClient(communicatorQueueClient),
        _state(eventServer, Topic::Connection) {}

    ConnectionState Connector::loop() {
        connect();
        delay(LoopDelayMilliSeconds);
        return _state;
    }

    void Connector::begin(const Configuration* configuration) {
        _state = ConnectionState::Init;
        _waitDuration = WifiInitialWaitDuration;
        _wifiConnectionFailureCount = 0;

        // what can be sent to the sampler (numerical payload)
        _eventServer->subscribe(_samplerQueueClient, Topic::BatchSizeDesired);
        _eventServer->subscribe(_samplerQueueClient, Topic::IdleRate);
        _eventServer->subscribe(_samplerQueueClient, Topic::NonIdleRate);
        _eventServer->subscribe(_samplerQueueClient, Topic::ResetSensor);

        _eventServer->subscribe(_communicatorDataQueue, Topic::Result);
        _eventServer->subscribe(_communicatorDataQueue, Topic::ConnectionError);
        _eventServer->subscribe(_communicatorDataQueue, Topic::Info);
        _eventServer->subscribe(_serializer, Topic::SensorData);

        // what can be sent to the communicator
        _eventServer->subscribe(_communicatorQueueClient, Topic::BatchSizeDesired);

        _eventServer->subscribe(_communicatorQueueClient, Topic::Connection);
        _eventServer->subscribe(_communicatorQueueClient, Topic::WifiSummaryReady);
        _eventServer->subscribe(_communicatorQueueClient, Topic::FreeQueueSize);
        _eventServer->subscribe(_communicatorQueueClient, Topic::FreeQueueSpaces);
        _eventServer->subscribe(_communicatorQueueClient, Topic::SetVolume);
        _eventServer->subscribe(_communicatorQueueClient, Topic::AddVolume);
        _eventServer->subscribe(_communicatorQueueClient, Topic::UpdateProgress);

        _wifi->configure(&configuration->ip);
    }

    ConnectionState Connector::connect() {
        // ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement -- all options handled
        switch (_state) {
        case ConnectionState::Init:
            handleInit();
            break;
        case ConnectionState::WifiConnecting:
            handleWifiConnecting();
            break;
        case ConnectionState::WifiConnected:
            handleWifiConnected();
            break;
        case ConnectionState::RequestTime:
            handleRequestTime();
            break;
        case ConnectionState::SettingTime:
            handleSettingTime();
            break;
        case ConnectionState::CheckFirmware:
            handleCheckFirmware();
            break;
        case ConnectionState::WifiReady:
            handleWifiReady();
            break;
        case ConnectionState::MqttConnecting:
            handleMqttConnecting();
            break;
        case ConnectionState::WaitingForMqttReconnect:
            handleWaitingForMqttReconnect();
            break;
        case ConnectionState::MqttConnected:
            handleMqttConnected();
            break;
        case ConnectionState::MqttReady:
            handleMqttReady();
            break;
        case ConnectionState::Disconnected:
            handleDisconnected();
        }
        return _state;
    }

    // private methods

    void Connector::handleCheckFirmware() {
        _firmwareManager->begin(_eventServer->request(Topic::MacRaw, ""));
        _firmwareManager->tryUpdate();
        _state = ConnectionState::WifiReady;
    }

    void Connector::handleDisconnected() {
        _state = ConnectionState::WifiConnecting;
        _waitDuration = WifiReconnectWaitDuration;
        _wifi->reconnect();
    }

    void Connector::handleInit() {
        _wifi->disconnect();
        _wifi->begin();
        _state = ConnectionState::WifiConnecting;
        _waitDuration = WifiInitialWaitDuration;
        _wifiConnectTimestamp = micros();
    }

    void Connector::handleMqttConnected() {
        if (!_mqttGateway->isConnected()) {
            _state = ConnectionState::WifiReady;
            return;
        }
        // don't announce if we already did it recently. 
        // using millis since micros works only a bit over 70 minutes.
        if (_lastAnnouncementTimestampMillis == 0 || millis() - _lastAnnouncementTimestampMillis > OneHourInMillis) {
            while (_mqttGateway->hasAnnouncement()) {
                _mqttGateway->publishNextAnnouncement();
            }
            _lastAnnouncementTimestampMillis = millis();
        }
        _mqttGateway->announceReady();
        _state = ConnectionState::MqttReady;
    }

    void Connector::handleMqttConnecting() {
        if (_mqttGateway->isConnected()) {
            _state = ConnectionState::MqttConnected;
            return;
        }
        _state = ConnectionState::WaitingForMqttReconnect;
    }

    void Connector::handleMqttReady() {
        if (!_wifi->isConnected()) {
            _state = ConnectionState::Disconnected;
            return;
        }
        if (!_mqttGateway->isConnected()) {
            _state = ConnectionState::WifiReady;
            return;
        }

        while (_samplerQueueClient->receive()) {}
        while (_communicatorQueueClient->receive()) {}

        // Retrieve a retained volume message from MQTT and pass it on to the communicator.
        // This should happen only once.
        if (_mqttGateway->getPreviousVolume()) {
            _eventServer->unsubscribe(_communicatorQueueClient, Topic::AddVolume);
        }

        // returns false if disconnected, minimizing risk of losing data from the queue
        if (!_mqttGateway->handleQueue()) {
            return;
        }
        DataQueuePayload* payload;
        while ((payload = _samplerDataQueue->receive()) != nullptr) {
            _eventServer->publish(Topic::SensorData, reinterpret_cast<const char*>(payload));
            if (payload->topic == Topic::Result) {
                _communicatorDataQueue->send(payload);
            }

        }
    }

    void Connector::handleRequestTime() {
        if (_timeServer->timeWasSet()) {
            _state = ConnectionState::CheckFirmware;
            return;
        }
        _timeServer->setTime();
        _state = ConnectionState::SettingTime;
        _requestTimeTimestamp = micros();
    }

    void Connector::handleSettingTime() {
        if (!_wifi->isConnected()) {
            _state = ConnectionState::Disconnected;
            return;
        }
        if (_timeServer->timeWasSet()) {
            _state = ConnectionState::CheckFirmware;
            return;
        }
        const unsigned long timeSetWaitDuration = micros() - _requestTimeTimestamp;
        if (timeSetWaitDuration > TimeserverWaitDuration) {
            // setting time failed. Retry.
            _state = ConnectionState::WifiConnected;
        }
    }

    void Connector::handleWaitingForMqttReconnect() {
        const unsigned long mqttWaitDuration = micros() - _mqttConnectTimestamp;
        if (mqttWaitDuration >= MqttReconnectWaitDuration) {
            _state = ConnectionState::WifiReady;
        }
    }

    void Connector::handleWifiConnected() {
        if (_wifi->needsReInit()) {
            _state = ConnectionState::Init;
            return;
        }
        if (!_wifi->isConnected()) {
            _state = ConnectionState::Disconnected;
            return;
        }
        _state = ConnectionState::RequestTime;
        _wifiConnectionFailureCount = 0;
    }

    void Connector::handleWifiConnecting() {
        if (_wifi->isConnected()) {
            _state = ConnectionState::WifiConnected;
            _wifiConnectionFailureCount = 0;
            _wifi->announceReady();
            return;
        }
        const unsigned long wifiWaitDuration = micros() - _wifiConnectTimestamp;
        if (wifiWaitDuration > _waitDuration) {
            _wifiConnectionFailureCount++;
            if (_wifiConnectionFailureCount > MaxReconnectFailures) {
                _state = ConnectionState::Init;
                _wifiConnectionFailureCount = 0;
                return;
            }
            _state = ConnectionState::Disconnected;
        }
    }

    void Connector::handleWifiReady() {
        if (_wifi->isConnected()) {
            _state = ConnectionState::MqttConnecting;
            _mqttConnectTimestamp = micros();
            // this is a synchronous call, takes a lot of time
            _mqttGateway->begin(_wifi->getHostName());
            return;
        }

        _state = ConnectionState::Disconnected;
    }

    // TODO: create new TaskExecutor class that contains task and virtual loop. Saves 14 coverage blocks

    [[ noreturn]] void Connector::task(void* parameter) {
        const auto me = static_cast<Connector*>(parameter);

        for (;;) {
            me->loop();
        }
    }
}