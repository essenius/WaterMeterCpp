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

#include "EventClient.h"
#include "EventServer.h"
#include "SafeCString.h"

EventClient::EventClient(EventServer* eventServer) : _eventServer(eventServer) {}

EventClient::~EventClient() {
    _eventServer->unsubscribe(this);
    _eventServer->cannotProvide(this);
}

void EventClient::update(const Topic topic, const long payload) {
    char numberBuffer[20];
    safeSprintf(numberBuffer, "%ld", payload);
    update(topic, numberBuffer);
}

void EventClient::update(const Topic topic, const Coordinate payload) {
    update(topic, payload.l);
}
