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

#ifndef HEADER_RESULTWRITER
#define HEADER_RESULTWRITER

#include "BatchWriter.h"
#include "SerialDriver.h"
#include "FlowMeter.h"
#include "Storage.h"

class ResultWriter : public BatchWriter{
public:
    ResultWriter(Storage *storage, int measureIntervalMicros);
    long getIdleFlushRate();
    long getNonIdleFlushRate();
    virtual bool needsFlush(bool endOfFile = false);
    virtual void prepareFlush();
    void setIdleFlushRate(long rate);
    void setIdleFlushRate(char* rateString);
    void setNonIdleFlushRate(long rate);
    void setNonIdleFlushRate(char* rateString);

    void addMeasurement(int value, int duration, FlowMeter *result);
    virtual char* getHeader();
protected:
    static const long FLUSH_RATE_IDLE = 6000L;
    static const long FLUSH_RATE_INTERESTING = 100L;
    long _driftCount = 0L;
    int _duration = 0;
    long _excludeCount = 0;
    long _flowCount = 0;
    long _idleFlushRate = FLUSH_RATE_IDLE;
    int _maxDuration = 0;
    int _measure = 0;
    int _measureIntervalMicros = 0;
    long _nonIdleFlushRate = FLUSH_RATE_INTERESTING;
    long _outlierCount = 0;
    long _overrunCount = 0;
    long _peakCount = 0;
    FlowMeter *_result = nullptr;
    Storage *_storage;
    float _sumAmplitude = 0;
    long _sumDuration = 0;
    float _sumLowPassOnHighPass = 0.0f;
    virtual void resetCounters();

};
#endif
