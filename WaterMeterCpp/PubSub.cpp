// Copyright 2021 Rik Essenius
// 
//   Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//   except in compliance with the License. You may obtain a copy of the License at
// 
//       http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software distributed under the License
//    is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and limitations under the License.

#include "PubSub.h"

#include "ArduinoMock.h"

EventClient::EventClient(const char* name, EventServer* eventServer) : _name(name), _eventServer(eventServer) {}

EventClient::~EventClient() {
    _eventServer->unsubscribe(this);
}

const char* EventClient::getName() {
    return _name;
}

EventServer::EventServer(LogLevel logLevel) : _logLevel(logLevel) {
    _numberBuffer[0] = '\0';
}

EventServer::EventServer() : EventServer(LogLevel::Off) {}

EventServer::~EventServer() {
    // should never be necessary, safety net
    auto iterator = _subscribers.begin();
    while (iterator != _subscribers.end()) {
        iterator->second.clear();
        iterator = _subscribers.erase(iterator);
    }
    _providers.clear();
}

void EventServer::cannotProvide(EventClient* client, Topic topic) {
    if (_providers[topic] == client) {
        _providers.erase(topic);
    }
}

void EventServer::cannotProvide(EventClient* client) {
    for (auto iterator = _providers.begin(); iterator != _providers.end(); ) {
        if (iterator->second == client) {
            iterator = _providers.erase(iterator);
        }
        else {
            iterator++;
        }
    }
}

void EventServer::provides(EventClient* client, Topic topic) {
    _providers[topic] = client;
}

void EventServer::publishLog(const char* format, EventClient* client, Topic topic, const char* payload) {
    if (_logLevel == LogLevel::On) {
        char buffer[255] = { 0 };
        sprintf(buffer, format, client->getName(), topic, payload);
        publish(nullptr, Topic::Info, buffer, false);
    }
}

void EventServer::publishLog(const char* format, EventClient* client, Topic topic, long payload) {
    if (_logLevel == LogLevel::On) {
        char buffer[20] = { 0 };
        sprintf(buffer, "%ld", payload);
        publishLog(format, client, topic, buffer);
    }
}

void EventServer::setLogLevel(LogLevel logLevel) {
    _logLevel = logLevel;
}

void EventServer::subscribe(EventClient* client, Topic topic) {
    publishLog("%s subscribes to %d\n", client, topic, 0L);
    for (auto& item : _subscribers) {
        if (item.first == topic) {
            // insert does not create duplicates
            item.second.insert(client);
            return;
        }
    }
    // topic not found, create a new entry
    std::set<EventClient*> subscribers;
    subscribers.insert(client);
    _subscribers[topic] = subscribers;
}

const char* EventServer::toString(long input) {
    sprintf(_numberBuffer, "%ld", input);
    return _numberBuffer;
}

// unsubscribe the client from all subscribed topics
// Note this is tricky as it could happen when another event is still being handled
void EventServer::unsubscribe(EventClient* client) {
    publishLog("%s unsubscribes\n", client, Topic::None, 0L);
    for (auto iterator = _subscribers.begin(); iterator != _subscribers.end(); ) {
        iterator->second.erase(client);
        if (iterator->second.size() == 0) {
            iterator = _subscribers.erase(iterator);
        }
        else {
            iterator++;
        }
    }
}

// unsubscribe the subscriber from the topic
void EventServer::unsubscribe(EventClient* client, Topic topic) {
    publishLog("%s unsubscribes from %d\n", client, topic, 0L);
    for (auto iterator = _subscribers.begin(); iterator != _subscribers.end(); iterator++) {
        if (iterator->first == topic) {
            iterator->second.erase(client);
            if (iterator->second.size() == 0) {
                // safe because we exit the loop (interator no longer valid after erase).
                _subscribers.erase(iterator);
            }
            return;
        }
    }
}
