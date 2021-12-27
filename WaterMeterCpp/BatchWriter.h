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

#ifndef HEADER_BATCHWRITER
#define HEADER_BATCHWRITER

#include "EventServer.h"
#include "TimeServer.h"
#include "PayloadBuilder.h"
#include "ChangePublisher.h"

using byte = unsigned char;

class BatchWriter : public EventClient {
public:
    BatchWriter(const char* name, EventServer* eventServer, TimeServer* timeServer, PayloadBuilder* payloadBuilder);
    virtual void begin(long desiredFlushRate);
    virtual void flush();
    long getFlushRate();
    const char* getMessage();
    virtual bool needsFlush(bool endOfFile = false);
    bool newMessage();
    virtual void prepareFlush();
    virtual void setDesiredFlushRate(long flushRate);

protected:
    TimeServer* _timeServer;
    virtual void initBuffer();
    long convertToLong(const char* stringParam, long defaultValue = 0L);
    long limit(long input, long min, long max);
    //void setFlushRate(long flushRate);
    virtual void update(Topic topic, long payload);
    bool _canFlush = true;
    long _desiredFlushRate = 0;
    //long _flushRate = 0;
    //Topic _flushRateTopic = Topic::Rate;
    bool _flushWaiting = false;
    virtual void resetCounters();

    PayloadBuilder* _payloadBuilder;
    long _messageCount = 0;
    ChangePublisher<long> _flushRatePublisher;
};

#endif
