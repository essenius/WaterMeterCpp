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

#ifndef HEADER_TIME_H
#define HEADER_TIME_H

#include "PubSub.h"

class TimeServer : public EventClient {
public:
    explicit TimeServer(EventServer* eventServer);
    void begin();
    virtual bool setTime();
    virtual const char* getTime();
    const char* get(Topic topic, const char* defaultValue) override;
    bool timeWasSet() const;

protected:
    static constexpr int BUFFER_SIZE = 26;
    static char _buffer[BUFFER_SIZE];
    bool _wasSet = false;
};
#endif
