// Copyright 2021-2024 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

// Gather results until the right number was received, and prepare the results for sending

#ifndef HEADER_RESULT_AGGREGATOR
#define HEADER_RESULT_AGGREGATOR

#include "Aggregator.h"
#include "FlowDetector.h"

namespace WaterMeter {
    class ResultAggregator final : public Aggregator {
    public:
        ResultAggregator(EventServer* eventServer, Clock* theClock, DataQueue* dataQueue, DataQueuePayload* payload,
            uint32_t measureIntervalMicros);
        void addDuration(unsigned long duration) const;
        void addMeasurement(const SensorSample& value, const FlowDetector* result);
        using Aggregator::begin;
        void begin();
        void flush() override;
        bool shouldSend(bool endOfFile = false) override;
        bool send() override;
        void update(Topic topic, const char* payload) override;
        void update(Topic topic, long payload) override;
        static constexpr int FlatlineStreak = 20;

    protected:
        static constexpr long FlushRateIdle = 6000L;
        static constexpr long FlushRateInteresting = 100L;

        ResultData* _result;
        long _idleFlushRate = FlushRateIdle;
        uint32_t _measureIntervalMicros = 0;
        long _nonIdleFlushRate = FlushRateInteresting;
        uint32_t _streak = 1;

        void setIdleFlushRate(long rate);
        void setNonIdleFlushRate(long rate);
    };
}
#endif
