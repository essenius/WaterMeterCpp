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

#ifndef HEADER_DEVICE_H
#define HEADER_DEVICE_H

#include "EventServer.h"
#include "LongChangePublisher.h"

class Device : public EventClient {
public:
    explicit Device(EventServer* eventServer);
    void begin();
    void reportHealth();
private:
    LongChangePublisher _freeHeap;
    LongChangePublisher _freeStack;

    long freeHeap();
    long freeStack();
};
#endif
