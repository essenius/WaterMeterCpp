// Copyright 2022 Rik Essenius
// 
//   Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//   except in compliance with the License. You may obtain a copy of the License at
// 
//       http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software distributed under the License
//    is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and limitations under the License.

#ifndef HEADER_CLOCK_H
#define HEADER_CLOCK_H

#ifdef ESP32
#include <ESP.h>  
#include <sys/time.h>
#else
#include <ctime>
#include <cstdio>
struct timeval {
    time_t tv_sec; // seconds 
    long tv_usec; // microseconds
};
#endif

typedef unsigned long long Timestamp;

class Clock {
    public:

    static Timestamp getTimestamp();
    static bool formatTimestamp(Timestamp timestamp, char* destination, size_t size);
};
#endif
