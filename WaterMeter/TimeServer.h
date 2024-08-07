// Copyright 2021-2023 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

// Ensures that the system uses the accurate time. This is important to be able to use TLS certificates.

#ifndef HEADER_TIMESERVER
#define HEADER_TIMESERVER

namespace WaterMeter {
    class TimeServer {
    public:
        TimeServer() = default;
        virtual ~TimeServer() = default;
        TimeServer(const TimeServer&) = default;
        TimeServer(TimeServer&&) = default;
        TimeServer& operator=(const TimeServer&) = default;
        TimeServer& operator=(TimeServer&&) = default;

        virtual void setTime();
        virtual bool timeWasSet() const;

    protected:
        bool _wasSet = false;
    };
}
#endif
