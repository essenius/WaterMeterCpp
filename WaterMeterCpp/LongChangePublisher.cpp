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
    EventServer* eventServer, /*EventClient* eventClient,*/ const Topic topic, const long epsilon, const long lowThreshold, int8_t index) :
    ChangePublisher(eventServer, /*eventClient,*/ topic, index), _epsilon(epsilon), _lowThreshold(lowThreshold) {}

LongChangePublisher& LongChangePublisher::operator=(long payload) {
    // Only catch larger variations or values close to a critical value to avoid very frequent updates
    if (abs(_payload - payload) > _epsilon || payload < _lowThreshold) {
        ChangePublisher::operator=(payload);
/*        _eventServer->publish(_topic, payload);
        _payload = payload; */
    }
    return *this;
}
