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

#ifndef HEADER_INTCHANGEPUBLISHER
#define HEADER_INTCHANGEPUBLISHER

#include "EventServer.h"
#include "ChangePublisher.h"

class LongChangePublisher : public ChangePublisher<long>
{
public:
    LongChangePublisher(EventServer* eventServer, EventClient* eventClient, Topic topic, long epsilon, long lowThreshold);
    void set(long payload) override;

protected:
    long _epsilon;
    long _lowThreshold;
};

#endif
