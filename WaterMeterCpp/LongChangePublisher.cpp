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

#include "LongChangePublisher.h"
#ifdef ESP32
#include <ESP.h>
#endif

LongChangePublisher::LongChangePublisher(
    EventServer* eventServer, const Topic topic, const long epsilon, const long lowThreshold, int8_t index) :
    ChangePublisher(eventServer, topic, index), _epsilon(epsilon), _lowThreshold(lowThreshold) {}

LongChangePublisher& LongChangePublisher::operator=(long payload) {
    // Only catch larger variations or values close to a critical value to avoid very frequent updates
    if (_epsilon == 1 && _payload != payload) {
        ChangePublisher::operator=(payload);
        return *this;
    }
    if (payload < _lowerLimit || payload > _upperLimit || payload < _lowThreshold) {
        ChangePublisher::operator=(payload);
        _lowerLimit = (_payload / _epsilon) * _epsilon;
        _upperLimit = _lowerLimit + _epsilon;
    }
    return *this;
}
