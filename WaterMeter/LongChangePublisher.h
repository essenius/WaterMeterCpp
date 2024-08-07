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

// Like Change Publisher, it sends an event only when its payload changes.
// It works with long integers, and it can deal with thresholds and multiples to reduce the number of times it fires.
// This allows for e.g. not reporting each and every change to the stack size, but only if it differs more than 1kB.

#ifndef HEADER_LONG_CHANGE_PUBLISHER
#define HEADER_LONG_CHANGE_PUBLISHER

#include <climits>
#include "EventServer.h"
#include "ChangePublisher.h"

namespace WaterMeter {
    class LongChangePublisher final : public ChangePublisher<long> {
    public:
        LongChangePublisher(
            EventServer* eventServer,
            Topic topic,
            long epsilon,
            long lowThreshold,
            int8_t index = 0,
            long defaultValue = 0);
        LongChangePublisher& operator=(long payload) override;

    protected:
        long _epsilon;
        long _lowThreshold;
        long _lowerLimit = LONG_MAX;
        long _upperLimit = LONG_MIN;

    };
}
#endif
