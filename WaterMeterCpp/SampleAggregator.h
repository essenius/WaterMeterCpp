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

// Gather a batch of samples and prepare them for sending ocer

#ifndef HEADER_SAMPLEAGGREGATOR
#define HEADER_SAMPLEAGGREGATOR

#include "Aggregator.h"

class SampleAggregator final : public Aggregator {
public:
    SampleAggregator(EventServer* eventServer, Clock* theClock, DataQueue* dataQueue, DataQueuePayload* payload);
    using Aggregator::begin;
    void addSample(const IntCoordinate& sample);
    void begin();
    void flush() override;
    void update(Topic topic, const char* payload) override;
    void update(Topic topic, long payload) override;
    void update(Topic topic, IntCoordinate payload) override;
protected:
    static constexpr unsigned char DefaultFlushRate = 25;
    static constexpr unsigned char MaxFlushRate = 25;
    uint16_t _currentSample = 0;
};

#endif
