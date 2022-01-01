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

#include "BinaryStatusPublisher.h"

BinaryStatusPublisher::BinaryStatusPublisher(
    EventServer* eventServer, EventClient* eventClient, Topic offTopic, Topic onTopic) :
    ChangePublisher(eventServer, eventClient, onTopic), _offTopic(offTopic), _onTopic(onTopic) {}

void BinaryStatusPublisher::set(bool payload) {
    setTopic(payload ? _onTopic : _offTopic);
    ChangePublisher::set(payload);
}
