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

#include <ESP.h>
#include "LongChangePublisher.h"

LongChangePublisher::LongChangePublisher(
    EventServer* eventServer,
    const Topic topic,
    const long epsilon,
    const long lowThreshold,
    const int8_t index,
    const long defaultValue) :
    ChangePublisher(eventServer, topic, index, defaultValue), _epsilon(epsilon), _lowThreshold(lowThreshold) {}

LongChangePublisher& LongChangePublisher::operator=(const long payload) {
    // Only catch larger variations or values close to a critical value to avoid very frequent updates
    if (_epsilon == 1 && _payload != payload) {
        ChangePublisher::operator=(payload);
        return *this;
    }
    if (payload < _lowerLimit || payload > _upperLimit || payload < _lowThreshold) {
        ChangePublisher::operator=(payload);
        // integer arithmetic. Find the closest multiple of _epsilon
        const auto roundedValue = (2 * _payload + _epsilon) / (2 * _epsilon) * _epsilon;
        _lowerLimit = roundedValue - _epsilon;
        _upperLimit = roundedValue + _epsilon;
    }
    return *this;
}
