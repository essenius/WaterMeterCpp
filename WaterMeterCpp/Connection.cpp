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

#include "Connection.h"

constexpr const char* LOST_WIFI_CONNECTION = "lost Wifi connection";
constexpr const char* LOST_BROKER_CONNECTION = "Lost MQTT broker connection";
constexpr const char* FAILED_BROKER_CONNECTION = "Failed to connect to MQTT broker";
constexpr const char* FAILED_WIFI_CONNECTION = "Could not connect to wifi. Reconnecting";
constexpr const char* REINIT_WIFI_CONNECTION = "Max wifi reconnect attempts reached. Initializing";

Connection::Connection(EventServer* eventServer, Wifi* wifi, MqttGateway* mqttGatway) :
    _eventServer(eventServer), _state(ConnectionState::Init), _wifi(wifi), _mqttGateway(mqttGatway) {}

void Connection::begin() {
    _state = ConnectionState::Init;
    _waitDuration = WIFI_INITIAL_WAIT_DURATION;
    _wifiConnectionFailureCount = 0;
}

ConnectionState Connection::connectWifi() {
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
    case ConnectionState::WifiReady:
        handleWifiReadyForSetup();
        break;
    case ConnectionState::Disconnected:
    default:
        handleDisconnected();
    }
    return _state;
}

ConnectionState Connection::connectMqtt() {
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
    case ConnectionState::WifiReady:
        handleWifiReady();
        break;
    case ConnectionState::MqttConnecting:
        handleMqttConnecting();
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

void Connection::handleInit() {
    _wifi->disconnect();
    _wifi->begin();
    _state = ConnectionState::WifiConnecting;
    _eventServer->publish(Topic::ConnectingWifi, LONG_TRUE);
    _waitDuration = WIFI_INITIAL_WAIT_DURATION;
    _wifiConnectTimestamp = micros();
}

void Connection::handleWifiConnecting() {
    if (_wifi->isConnected()) {
        _state = ConnectionState::WifiConnected;
        _wifiConnectionFailureCount = 0;
        return;
    }
    const unsigned long wifiWaitDuration = micros() - _wifiConnectTimestamp;
    if (wifiWaitDuration > _waitDuration) {
        _wifiConnectionFailureCount++;
        if (_wifiConnectionFailureCount > MAX_RECONNECT_FAILURES) {
            _eventServer->publish(Topic::Error, REINIT_WIFI_CONNECTION);
            _state = ConnectionState::Init;
            _wifiConnectionFailureCount = 0;
            return;
        }
        _eventServer->publish(Topic::Error, FAILED_WIFI_CONNECTION);
        _state = ConnectionState::Disconnected;
    }
}

void Connection::handleWifiConnected() {
    if (_wifi->needsReinit()) {
        _state = ConnectionState::Init;
        return;
    }
    _wifi->announceReady();
    _state = ConnectionState::WifiReady;
    _wifiConnectionFailureCount = 0;
}

void Connection::handleWifiReadyForSetup() {
    if (!_wifi->isConnected()) {
        _eventServer->publish(Topic::Error, LOST_WIFI_CONNECTION);
        _state = ConnectionState::Disconnected;
    }
}

void Connection::handleWifiReady() {
    if (_wifi->isConnected()) {
        _state = ConnectionState::MqttConnecting;
        _eventServer->publish(Topic::ConnectingMqtt, LONG_TRUE);
        _mqttConnectTimestamp = micros();
        // this is a synchronous call, takes a lot of time
        _mqttGateway->begin(_wifi->getClient(), _wifi->getHostName());
        return;
    }
    _eventServer->publish(Topic::Error, LOST_WIFI_CONNECTION);

    _state = ConnectionState::Disconnected;
}

void Connection::handleMqttConnecting() {
    if (_mqttGateway->isConnected()) {
        _state = ConnectionState::MqttConnected;
        return;
    }
    _eventServer->publish(Topic::Error, FAILED_BROKER_CONNECTION);
    // don't reconnect right away if the first time failed.
    const unsigned long mqttWaitDuration = micros() - _mqttConnectTimestamp;
    if (mqttWaitDuration > MQTT_RECONNECT_WAIT_DURATION) {
        _state = ConnectionState::WifiReady;
    }
}

void Connection::handleMqttConnected() {
    if (!_mqttGateway->isConnected()) {
        _eventServer->publish(Topic::Error, LOST_BROKER_CONNECTION);
        _state = ConnectionState::WifiReady;
    }
    // The connection is ready when we are done with the announcements
    if (_mqttGateway->hasAnnouncement()) {
        _mqttGateway->publishNextAnnouncement();
    } else {
        _mqttGateway->announceReady();
        _eventServer->publish(Topic::Connected, LONG_TRUE);
        _state = ConnectionState::MqttReady;
    }
}

void Connection::handleMqttReady() {
    if (!_wifi->isConnected()) {
        _state = ConnectionState::Disconnected;
        _eventServer->publish(Topic::Error, LOST_WIFI_CONNECTION);
        _eventServer->publish(Topic::Disconnected, LONG_TRUE);

        return;
    }
    if (!_mqttGateway->isConnected()) {
        _state = ConnectionState::WifiReady;
        _eventServer->publish(Topic::Error, LOST_BROKER_CONNECTION);
        _eventServer->publish(Topic::Disconnected, LONG_TRUE);
    }
}

void Connection::handleDisconnected() {
    _state = ConnectionState::WifiConnecting;
    _eventServer->publish(Topic::ConnectingWifi, LONG_TRUE);
    _waitDuration = WIFI_RECONNECT_WAIT_DURATION;
    _wifi->reconnect();
}
