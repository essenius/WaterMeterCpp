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

#include "EventClient.h"
#include "PubSub.h"

EventClient::EventClient(const char* name, EventServer* eventServer) : _name(name), _eventServer(eventServer) {}

EventClient::~EventClient() {
    _eventServer->unsubscribe(this);
    _eventServer->cannotProvide(this);
}

const char* EventClient::getName() {
    return _name;
}

void EventClient::update(Topic topic, long payload) {
    char numberBuffer[20];
    sprintf(numberBuffer, "%ld", payload);
    update(topic, numberBuffer);
}
