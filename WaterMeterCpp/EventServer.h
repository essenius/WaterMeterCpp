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

#ifndef HEADER_EVENTSERVER
#define HEADER_EVENTSERVER

#include "EventClient.h"
#include <cstdarg>
#include <map>
#include <set>

class EventServer {
public:
    EventServer();
    // No need for a destructor. Clients clean up when destroyed, and do so before the server gets destroyed.
    // Deleting the server before the client would cause an access violation when the client gets destroyed.

    explicit EventServer(LogLevel logLevel);
    void provides(EventClient* e, Topic topic);
    void cannotProvide(EventClient* e, Topic topic);
    void cannotProvide(EventClient* e);
    void setLogLevel(LogLevel logLevel);
    void subscribe(EventClient* e, Topic topic);
    void unsubscribe(EventClient* e, Topic topic);
    void unsubscribe(EventClient* e);

    // Request a topic. There can be only one provider
    template<class payloadType> payloadType request(Topic topic, payloadType defaultValue) {
        auto provider = _providers.find(topic);
        if (provider != _providers.end()) {
            auto eventClient = provider->second;
            publishLog("%s provides %d\n", eventClient, topic, 0L);
            return eventClient->get(topic, defaultValue);
        }
        return defaultValue;
    }

    // Publish to all subscribers except the sender
    template<class payloadType> void publish(EventClient* client, Topic topic, payloadType payload, bool log = true) {
        auto subscribers = _subscribers.find(topic);
        if (subscribers != _subscribers.end()) {
            for (auto eventClient : subscribers->second) {
                if (client != eventClient && !eventClient->isMuted()) {
                    if (_logLevel == LogLevel::On && log) {
                        publishLog("Updating %s on %d with '%s'\n", eventClient, topic, payload);
                    }
                    eventClient->update(topic, payload);
                }
            }
        }
    }

    // Publish to all subscribers including the sender
    template<class payloadType> void publish(Topic topic, payloadType payload) {
        publish(NULL, topic, payload);
    }

private:
    void publishLog(const char* format, EventClient* client, Topic topic, const char* payload);
    void publishLog(const char* format, EventClient* client, Topic topic, long payload);

    std::map<Topic, std::set<EventClient*>> _subscribers;
    std::map<Topic, EventClient*> _providers;
    LogLevel _logLevel = LogLevel::Off;
    char _numberBuffer[10];
};

#endif
