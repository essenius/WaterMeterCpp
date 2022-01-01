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

LongChangePublisher::LongChangePublisher(EventServer* eventServer, EventClient* eventClient, Topic topic, long epsilon, long lowThreshold) :
    ChangePublisher(eventServer, eventClient, topic), _epsilon(epsilon), _lowThreshold(lowThreshold) {}

void LongChangePublisher::set(long payload) {
    // Only catch larger variations to avoid very frequent updates
    if (abs(_payload - payload) >= _epsilon || payload < _lowThreshold) {
        _eventServer->publish(_topic, payload);
            _payload = payload;
    }
}
