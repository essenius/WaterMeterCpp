// Copyright 2021-2022 Rik Essenius
// 
//   Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//   except in compliance with the License. You may obtain a copy of the License at
// 
//       http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software distributed under the License
//    is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and limitations under the License.

#ifndef HEADER_CHANGEPUBLISHER
#define HEADER_CHANGEPUBLISHER

#include "EventServer.h"

template <class payloadType>
class ChangePublisher {
public:
    ChangePublisher(EventServer* eventServer, EventClient* eventClient, Topic topic) {
        _eventServer = eventServer;
        _eventClient = eventClient;
        _topic = topic;
    }

    payloadType get() { return _payload; }

    virtual void set(payloadType payload) {
        if (payload != _payload) {
            _payload = payload;
            _eventServer->publish<payloadType>(_eventClient, _topic, _payload);
        }
    }

    void reset() { _payload = payloadType(); }
    void setTopic(Topic topic) { _topic = topic; }

protected:
    EventServer* _eventServer;
    EventClient* _eventClient;
    payloadType _payload{};
    Topic _topic;
};

#endif
