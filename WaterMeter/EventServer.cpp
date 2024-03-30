// Copyright 2021-2024 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#include "EventServer.h"

#include <ESP.h>

namespace WaterMeter {
    EventServer::EventServer() : _numberBuffer{ 0 } {}

    void EventServer::cannotProvide(const EventClient* client, const Topic topic) {
        if (_providers[topic] == client) {
            _providers.erase(topic);
        }
    }

    void EventServer::cannotProvide(const EventClient* client) {
        for (auto iterator = _providers.begin(); iterator != _providers.end();) {
            if (iterator->second == client) {
                iterator = _providers.erase(iterator);
            }
            else {
                ++iterator;
            }
        }
    }

    void EventServer::provides(EventClient* client, const Topic topic) {
        _providers[topic] = client;
    }

    void EventServer::subscribe(EventClient* client, const Topic topic) {
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

    // unsubscribe the client from all subscribed topics
    // Note this is tricky as it could happen when another event is still being handled
    void EventServer::unsubscribe(EventClient* client) {
        for (auto iterator = _subscribers.begin(); iterator != _subscribers.end();) {
            iterator->second.erase(client);
            if (iterator->second.empty()) {
                iterator = _subscribers.erase(iterator);
            }
            else {
                ++iterator;
            }
        }
    }

    // unsubscribe the subscriber from the topic
    void EventServer::unsubscribe(EventClient* client, const Topic topic) {
        for (auto iterator = _subscribers.begin(); iterator != _subscribers.end(); ++iterator) {
            if (iterator->first == topic) {
                iterator->second.erase(client);
                if (iterator->second.empty()) {
                    // safe because we exit the loop (iterator no longer valid after erase).
                    _subscribers.erase(iterator);
                }
                return;
            }
        }
    }
}