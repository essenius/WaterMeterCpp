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

#ifndef HEADER_MEASUREMENTWRITER
#define HEADER_MEASUREMENTWRITER

#include "BatchWriter.h"
#include "EventServer.h"

class MeasurementWriter : public BatchWriter {
public:
    MeasurementWriter(EventServer* eventServer, PayloadBuilder* payloadBuilder);
    using BatchWriter::begin;
    virtual void begin();
    void addMeasurement(int measure);
    void prepareFlush() override;
    void update(Topic topic, const char* payload) override;
    void update(Topic topic, long payload) override;
protected:
    void initBuffer() override;
    static constexpr unsigned char DEFAULT_FLUSH_RATE = 50;
    static constexpr unsigned char MAX_FLUSH_RATE = 60;
};

#endif
