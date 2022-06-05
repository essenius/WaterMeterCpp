// Copyright 2021-2022 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#ifndef HEADER_AGGREGATOR
#define HEADER_AGGREGATOR

#include "Clock.h"
#include "DataQueuePayload.h"
#include "ChangePublisher.h"
#include "DataQueue.h"

class Aggregator : public EventClient {
public:
    Aggregator(EventServer* eventServer, Clock* theClock, DataQueue* dataQueue, DataQueuePayload* payload);
    virtual void begin(long desiredFlushRate);
    bool canSend() const;
    virtual void flush();
    long getFlushRate();
    bool newMessage();
    DataQueuePayload* getPayload() const;
    virtual bool send();
    virtual void setDesiredFlushRate(long flushRate);
    virtual bool shouldSend(bool force = false);

protected:
    static long convertToLong(const char* stringParam, long defaultValue = 0L);
    static long limit(long input, long min, long max);
    Clock* _clock;
    DataQueue* _dataQueue;
    DataQueuePayload* _payload;
    ChangePublisher<long> _flushRate;
    ChangePublisher<long> _blocked;
    long _desiredFlushRate = 0;
    long _messageCount = 0;
};

#endif
