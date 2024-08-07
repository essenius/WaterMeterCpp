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

// Simple implementation of an event server supporting publish/subscribe and request/response patterns

#ifndef HEADER_EVENT_SERVER
#define HEADER_EVENT_SERVER

#include "EventClient.h"
#include <map>
#include <set>

namespace WaterMeter {
    class EventServer {
    public:
        EventServer();
        // No need for a destructor. Clients clean up when destroyed, and do so before the server gets destroyed.
        // Deleting the server before the client would cause an access violation when the client gets destroyed.

        void provides(EventClient* client, Topic topic);
        void cannotProvide(const EventClient* client, Topic topic);
        void cannotProvide(const EventClient* client);

        // Request a topic. There can be only one provider
        template <class PayloadType>
        PayloadType request(Topic topic, PayloadType defaultValue) {
            const auto provider = _providers.find(topic);
            if (provider != _providers.end()) {
                const auto eventClient = provider->second;
                return eventClient->get(topic, defaultValue);
            }
            return defaultValue;
        }

        // Publish to all subscribers except the sender
        template <class PayloadType>
        void publish(EventClient* client, Topic topic, PayloadType payload) {
            const auto subscribers = _subscribers.find(topic);
            if (subscribers != _subscribers.end()) {
                for (auto eventClient : subscribers->second) {
                    if (client != eventClient) {
                        eventClient->update(topic, payload);
                    }
                }
            }
        }

        // Publish to all subscribers including the sender
        template <class PayloadType>
        void publish(Topic topic, PayloadType payload) {
            publish(NULL, topic, payload);
        }

        void subscribe(EventClient* client, Topic topic);
        void unsubscribe(EventClient* client, Topic topic);
        void unsubscribe(EventClient* client);

    private:
        char _numberBuffer[10];
        std::map<Topic, EventClient*> _providers;
        std::map<Topic, std::set<EventClient*>> _subscribers;
    };
}
#endif
