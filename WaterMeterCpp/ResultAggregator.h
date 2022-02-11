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

#ifndef HEADER_RESULTAGGREGATOR
#define HEADER_RESULTAGGREGATOR

#include "Aggregator.h"
#include "FlowMeter.h"
#include "EventServer.h"

class ResultAggregator : public Aggregator {
public:
    ResultAggregator(EventServer* eventServer, Clock* theClock, DataQueue* dataQueue, SensorDataQueuePayload* payload,
                     uint32_t measureIntervalMicros);
    void addDuration(uint32_t duration);
    void addMeasurement(int16_t value, const FlowMeter* result);
    using Aggregator::begin;
    virtual void begin();
    void flush() override;
    bool shouldSend(bool endOfFile = false) override;
    bool send() override;
    void update(Topic topic, const char* payload) override;
    void update(Topic topic, long payload) override;
    static constexpr int FLATLINE_STREAK = 20;

protected:
    static constexpr long FLUSH_RATE_IDLE = 6000L;
    static constexpr long FLUSH_RATE_INTERESTING = 100L;

    ResultData* _result;
    uint32_t _streak = 1;
    uint32_t _measureIntervalMicros = 0;
    ChangePublisher<long> _overrun;
    long _idleFlushRate = FLUSH_RATE_IDLE;
    long _nonIdleFlushRate = FLUSH_RATE_INTERESTING;
    void setIdleFlushRate(long rate);
    void setNonIdleFlushRate(long rate);
};


#endif
