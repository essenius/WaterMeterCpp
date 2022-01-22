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

#include "Connector.h"

#include "LedDriver.h"
#include "Log.h"
#include "QueueClient.h"
#include "secrets.h" // for the CONFIG constants

Connector::Connector(EventServer* eventServer, Log* logger, LedDriver* ledDriver, Wifi* wifi, MqttGateway* mqttGatway, 
    TimeServer* timeServer, FirmwareManager* firmwareManager, DataQueue* dataQueue, QueueClient* queueClient) :
    EventClient(eventServer),
    _logger(logger),
    _ledDriver(ledDriver),
    _wifi(wifi),
    _mqttGateway(mqttGatway),
    _timeServer(timeServer),
    _firmwareManager(firmwareManager),
    _dataQueue(dataQueue),
    _queueClient(queueClient),
    _state(eventServer, this, Topic::Connection)
{}

void Connector::setup() {
    _logger->begin();
    _ledDriver->begin();
    _state = ConnectionState::Init;
    _waitDuration = WIFI_INITIAL_WAIT_DURATION;
    _wifiConnectionFailureCount = 0;

    _eventServer->subscribe(_queueClient, Topic::BatchSizeDesired);
    _eventServer->subscribe(_queueClient, Topic::IdleRate);
    _eventServer->subscribe(_queueClient, Topic::NonIdleRate);
    _eventServer->subscribe(_queueClient, Topic::ResetSensor);


#ifdef CONFIG_USE_STATIC_IP
    _wifi->configure(CONFIG_LOCAL_IP, CONFIG_GATEWAY, CONFIG_SUBNETMASK, CONFIG_DNS_1, CONFIG_DNS_2);
#else
    _wifi->configure(Wifi::NO_IP, Wifi::NO_IP, Wifi::NO_IP, Wifi::NO_IP, Wifi::NO_IP);
#endif

#ifdef CONFIG_USE_TLS
    _wifi->setCertificates(CONFIG_ROOTCA_CERTIFICATE, CONFIG_DEVICE_CERTIFICATE, CONFIG_DEVICE_PRIVATE_KEY);
#endif

}

ConnectionState Connector::loop() {
    const auto state = connectMqtt();
    delay(100);
    return state;
}

void Connector::task(void *parameter) {
    const auto me = static_cast<Connector*>(parameter);

    for (;;) {
        me->loop();
    }
}

ConnectionState Connector::connectMqtt() {
    switch (_state.get()) {
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
    return _state.get();
}

void Connector::handleInit() {
    _wifi->disconnect();
    _wifi->begin();
    _state = ConnectionState::WifiConnecting;
    _waitDuration = WIFI_INITIAL_WAIT_DURATION;
    _wifiConnectTimestamp = micros();
}

void Connector::handleWifiConnecting() {
    if (_wifi->isConnected()) {
        _state = ConnectionState::WifiConnected;
        _wifiConnectionFailureCount = 0;
        return;
    }
    const unsigned long wifiWaitDuration = micros() - _wifiConnectTimestamp;
    if (wifiWaitDuration > _waitDuration) {
        _wifiConnectionFailureCount++;
        if (_wifiConnectionFailureCount > MAX_RECONNECT_FAILURES) {
            _state = ConnectionState::Init;
            _wifiConnectionFailureCount = 0;
            return;
        }
        _state = ConnectionState::Disconnected;
    }
}

void Connector::handleWifiConnected() {
    if (_wifi->needsReinit()) {
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

void Connector::handleRequestTime() {
    if (_timeServer->timeWasSet()) {
        _state = ConnectionState::CheckFirmware;
        return;
    }
    _timeServer->begin();
    _state = ConnectionState::SettingTime;
    _requestTimeTimestamp = micros();
}

void Connector::handleSettingTime() {
    if (_timeServer->timeWasSet()) {
        _wifi->announceReady();
        _state = ConnectionState::CheckFirmware;
        return;
    }

    const unsigned long timesetWaitDuration = micros() - _requestTimeTimestamp;
    if (timesetWaitDuration > TIMESERVER_WAIT_DURATION) {
        // setting time failed. Retry.
        _state = ConnectionState::WifiConnected;
    }
}

void Connector::handleCheckFirmware() {
    _firmwareManager->begin(_wifi->getClient(), _eventServer->request(Topic::MacRaw,""));
    _firmwareManager->tryUpdate();
    _state = ConnectionState::WifiReady;
}

void Connector::handleWifiReady() {
    if (_wifi->isConnected()) {
        _state = ConnectionState::MqttConnecting;
        _mqttConnectTimestamp = micros();
        // this is a synchronous call, takes a lot of time
        _mqttGateway->begin(_wifi->getClient(), _wifi->getHostName());
        return;
    }

    _state = ConnectionState::Disconnected;
}

void Connector::handleMqttConnecting() {
    if (_mqttGateway->isConnected()) {
        _state = ConnectionState::MqttConnected;
        return;
    }
    _state = ConnectionState::WaitingForMqttReconnect;
}

void Connector::handleWaitingForMqttReconnect() {
    const unsigned long mqttWaitDuration = micros() - _mqttConnectTimestamp;
    if (mqttWaitDuration >= MQTT_RECONNECT_WAIT_DURATION) {
        _state = ConnectionState::WifiReady;
    }
}

void Connector::handleMqttConnected() {
    if (!_mqttGateway->isConnected()) {
        _state = ConnectionState::WifiReady;
        return;
    }
    // The connection is ready when we are done with the announcements
    if (_mqttGateway->hasAnnouncement()) {
        _mqttGateway->publishNextAnnouncement();
    } else {
        _mqttGateway->announceReady();
        _state = ConnectionState::MqttReady;
    }
}

void Connector::handleMqttReady() {
    if (!_wifi->isConnected()) {
        _state = ConnectionState::Disconnected;
        return;
    }
    if (!_mqttGateway->isConnected()) {
        _state = ConnectionState::WifiReady;
    }
    // process all waiting batches that we need to send out
    while (_dataQueue->receive()) {}
}

void Connector::handleDisconnected() {
    _state = ConnectionState::WifiConnecting;
    _waitDuration = WIFI_RECONNECT_WAIT_DURATION;
    _wifi->reconnect();
}
