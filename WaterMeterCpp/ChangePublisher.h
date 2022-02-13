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

// ChangePublisher records a value and publishes the value only when it changes.
// The (8 bit) index is used to be able to share the same topic for multiple entities, e.g. different queues.
// This works by using the highest significant byte of the (4 byte) long to store the index.
// That means 3 bytes (max value 8,388,608 for long) are left for the payload if the index is used.

template <class payloadType>
class ChangePublisher {
public:
    ChangePublisher(EventServer* eventServer, /*EventClient* eventClient, */ Topic topic, int8_t index = 0) {
        _eventServer = eventServer;
        /*_eventClient = eventClient;*/
        _index = index << 24;
        _topic = topic;
    }

    ChangePublisher(const ChangePublisher&) = default;
    ChangePublisher(ChangePublisher&&) = default;
    ChangePublisher& operator=(const ChangePublisher&) = default;
    ChangePublisher& operator=(ChangePublisher&&) = default;
    virtual ~ChangePublisher() = default;

    operator payloadType() const { return _payload; }

    void reset() { _payload = payloadType(); }
    void setTopic(const Topic topic) { _topic = topic; }

    virtual ChangePublisher& operator=(payloadType payload) {
        if (payload != _payload) {
            _payload = payload;
            _eventServer->publish(/*_eventClient,*/ _topic, static_cast<long>(payload) + _index);
        }
        return *this;
    }

protected:
    EventServer* _eventServer;
    /*EventClient* _eventClient;*/
    long _index;
    payloadType _payload{};
    Topic _topic;
};

#endif
