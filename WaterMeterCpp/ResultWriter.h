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

#include "BatchWriter.h"
#include "FlowMeter.h"
#include "EventServer.h"

class ResultWriter : public BatchWriter {
public:
    ResultWriter(EventServer* eventServer, PayloadBuilder* payloadBuilder, int measureIntervalMicros);
    void addDuration(long duration);
    void addMeasurement(int value, const FlowMeter* result);
    using BatchWriter::begin;
    virtual void begin();
    bool needsFlush(bool endOfFile = false) override;
    void prepareFlush() override;
    void update(Topic topic, const char* payload) override;
    void update(Topic topic, long payload) override;

protected:
    static constexpr long FLUSH_RATE_IDLE = 6000L;
    static constexpr long FLUSH_RATE_INTERESTING = 100L;

    long _excludeCount = 0L;
    long _flowCount = 0L;
    long _idleFlushRate = FLUSH_RATE_IDLE;
    long _maxDuration = 0L;
    int _measure = 0;
    int _measureIntervalMicros = 0;
    long _nonIdleFlushRate = FLUSH_RATE_INTERESTING;
    long _outlierCount = 0L;
    bool _overrun = false;
    long _overrunCount = 0L;
    long _peakCount = 0L;
    long _processTime = 0L;
    float _smoothValue = 0.0f;
    float _derivative = 0.0f;
    float _smoothDerivative = 0.0f;
    float _smoothAbsDerivative = 0.0f;
    long _sumDuration = 0L;
    int _streak = 1;
    int _maxStreak = 1;
    int _previousMeasure = 0;

    //void publishOverrun(bool overrun);
    void resetCounters() override;
    void setIdleFlushRate(long rate);
    void setNonIdleFlushRate(long rate);
};
#endif
