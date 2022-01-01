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

#ifndef HEADER_SCHEDULER
#define HEADER_SCHEDULER

#include "BatchWriter.h"
#include "EventServer.h"

class Scheduler final : public EventClient {
public:
    Scheduler(EventServer* eventServer, BatchWriter* measureWriter, BatchWriter* resultWriter);
    bool processOutput();
private:
    static constexpr int MEASURE_ID = 0;
    static constexpr int RESULT_ID = 1;
    static constexpr int WRITER_COUNT = 2;

    bool _delayedFlush = false;
    bool _resultWritten = false;
    BatchWriter* _writer[WRITER_COUNT];

    int optimalWriteCount(int round);
};

#endif
