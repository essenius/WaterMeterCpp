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

#ifndef HEADER_BATCHWRITER
#define HEADER_BATCHWRITER

#include "RingbufferPayload.h"
#include "EventServer.h"
#include "ChangePublisher.h"
#include "DataQueue.h"

using byte = unsigned char;

class Aggregator : public EventClient {
public:
    Aggregator(EventServer* eventServer, DataQueue* dataQueue, RingbufferPayload* payload);
    virtual void begin(long desiredFlushRate);
    bool canSend() const;
    virtual void flush();
    long getFlushRate();
    RingbufferPayload* getPayload() const;
    //const char* getMessage() const;
    virtual bool shouldSend(bool force = false);
    bool newMessage();
    //virtual void prepareFlush();
    virtual bool send();
    virtual void setDesiredFlushRate(long flushRate);

protected:
    //virtual void initBuffer();
    static long convertToLong(const char* stringParam, long defaultValue = 0L);
    static long limit(long input, long min, long max);
    //void update(Topic topic, const char* payload) override;
    //void update(Topic topic, long payload) override;
    DataQueue* _dataQueue;
    //bool _canFlush = true;
    long _desiredFlushRate = 0;
    //bool _flushWaiting = false;
    //virtual void resetCounters();
    RingbufferPayload* _payload;
    long _messageCount = 0;
    ChangePublisher<long> _flushRate;
    ChangePublisher<long> _blocked;
};

#endif