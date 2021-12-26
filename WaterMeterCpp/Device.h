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

#ifndef HEADER_DEVICE_H
#define HEADER_DEVICE_H

#include <stdint.h>
#include "PubSub.h"

class Device : public EventClient {
public:
    Device(EventServer* eventServer);

    // Device does not subscribe, so no destructor needed

    void begin();
    void log(Topic topic);
    void reportHealth();
    void update(Topic topic, const char* payload);
    void update(Topic topic, long payload);
private:
    long _freeHeap = 0l;
    long _freeStack = 0L;
    long freeHeap();
    long freeStack();
    void reportFreeHeap();
    void reportFreeStack();
};
#endif
