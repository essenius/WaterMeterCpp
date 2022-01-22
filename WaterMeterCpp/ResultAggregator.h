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

#ifndef HEADER_RESULTWRITER
#define HEADER_RESULTWRITER

#include "Aggregator.h"
#include "FlowMeter.h"
#include "EventServer.h"

class ResultAggregator : public Aggregator {
public:
    ResultAggregator(EventServer* eventServer, DataQueue* dataQueue, RingbufferPayload* payload, uint32_t measureIntervalMicros);
    void addDuration(uint32_t duration);
    void addMeasurement(int16_t value, const FlowMeter* result);
    using Aggregator::begin;
    virtual void begin();
    void flush() override;
    bool shouldSend(bool endOfFile = false) override;
    //void prepareFlush() override;
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
    int16_t _previousMeasure = 0;
    ChangePublisher<long> _overrun;
    //long _excludeCount = 0L;
    //long _flowCount = 0L;
    long _idleFlushRate = FLUSH_RATE_IDLE;
    //long _maxDuration = 0L;
    //int _measure = 0;
    long _nonIdleFlushRate = FLUSH_RATE_INTERESTING;
    //long _outlierCount = 0L;
    //bool _overrun = false;
    //long _overrunCount = 0L;
    //long _peakCount = 0L;
    //long _processTime = 0L;
    //float _smoothValue = 0.0f;
    //float _derivative = 0.0f;
    //float _smoothDerivative = 0.0f;
    //float _smoothAbsDerivative = 0.0f;
    //long _sumDuration = 0L;
    //int _maxStreak = 1;

    //void publishOverrun(bool overrun);
    //void resetCounters() override;
    void setIdleFlushRate(long rate);
    void setNonIdleFlushRate(long rate);
};


#endif
