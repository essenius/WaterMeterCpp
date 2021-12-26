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

#ifndef HEADER_PUBSUB
#define HEADER_PUBSUB

#ifdef ESP32
#include <ESP.h>
#else
#endif
#include <cstdarg>
#include <map>
#include <set>

constexpr long LONG_TRUE = 1L;
constexpr long LONG_FALSE = 0L;

enum class Topic {
    None = 0,
    BatchSize, BatchSizeDesired, Rate, IdleRate, NonIdleRate,
    Sample, Measurement, Result,
    FreeHeap, FreeStack,
    Connected, Disconnected, Processing, Sending, DelayedFlush, ResultWritten,
    Error, Log, Info,
    ProcessTime, TimeOverrun,
    Flow, Exclude, Peak,
    Time
};

enum class LogLevel { Off = 0, On = 1 };

class EventServer;

class EventClient {
public:
    EventClient(const char* name, EventServer* eventServer);
    virtual ~EventClient();
    const char* getName();

    // can't use generics on virtual functions, which we would need here.
    // Since we use only a limited set of types, type erasure seems overkill 

    virtual const char* get(Topic topic, const char* defaultValue) { return defaultValue; }
    virtual long get(Topic topic, long defaultValue) { return defaultValue; }
    virtual void update(Topic topic, const char* payload) {}
    virtual void update(Topic topic, long payload) {}
    virtual void mute(bool muted) { _muted = muted; }
    virtual bool isMuted() { return _muted; }

protected:
    EventServer* _eventServer;
    const char* _name;
    bool _muted = false;
};

class EventServer {
public:
    EventServer();
    EventServer(LogLevel logLevel);
    virtual ~EventServer();
    void provides(EventClient* e, Topic topic);
    void cannotProvide(EventClient* e, Topic topic);
    void cannotProvide(EventClient* e);
    void setLogLevel(LogLevel logLevel);
    void subscribe(EventClient* e, Topic topic);
    void unsubscribe(EventClient* e, Topic topic);
    void unsubscribe(EventClient* e);

    // todo: refactor logging. Not so pretty right now

    const char* toString(const char* input) { return input; }
    const char* toString(long input);

    /*template<class payloadType>
    void log(const char* format, EventClient* client, Topic topic, payloadType payload) {
        if (_logLevel == LogLevel::On) {
            Serial.printf(format, client->getName(), static_cast<int>(topic), toString(payload));
        }
    }*/

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
