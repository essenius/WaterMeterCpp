// Copyright 2021-2023 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

// ChangePublisher records a value and publishes the value only when it changes.
// The (8 bit) index is used to be able to share the same topic for multiple entities, e.g. different queues.
// This works by using the highest significant byte of the (4 byte) long to store the index.
// That means 3 bytes (max value 8,388,608 for long) are left for the payload if the index is used.

#ifndef HEADER_CHANGEPUBLISHER
#define HEADER_CHANGEPUBLISHER

#include "EventServer.h"

template <class PayloadType>
class ChangePublisher {
public:
    ChangePublisher(
        EventServer* eventServer,
        const Topic topic,
        const int8_t index = 0,
        PayloadType defaultValue = PayloadType()):
        _eventServer(eventServer),
        _index(index << 24),
        _payload(defaultValue),
        _topic(topic) {}

    ChangePublisher(const ChangePublisher&) = default;
    ChangePublisher(ChangePublisher&&) = default;
    ChangePublisher& operator=(const ChangePublisher&) = default;
    ChangePublisher& operator=(ChangePublisher&&) = default;
    virtual ~ChangePublisher() = default;

    // ReSharper disable once CppNonExplicitConversionOperator -- done on purpose to be able to use as variable
    operator PayloadType() const { return _payload; }
    PayloadType get() const { return _payload; }

    void reset() { _payload = PayloadType(); }
    void setTopic(const Topic topic) { _topic = topic; }

    virtual ChangePublisher& operator=(PayloadType payload) {
        if (payload != _payload) {
            _payload = payload;
            _eventServer->publish(_topic, static_cast<long>(payload) + _index);
        }
        return *this;
    }

protected:
    EventServer* _eventServer;
    long _index;
    PayloadType _payload;
    Topic _topic;
};

#endif
