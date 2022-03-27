// Copyright 2022 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

// Mock implementation for unit testing (not targeting the ESP32)

// Disabling warnings caused by mimicking existing interface
// ReSharper disable CppInconsistentNaming
// ReSharper disable CppNonInlineFunctionDefinitionInHeaderFile

#ifndef HEADER_TIME
#define HEADER_TIME
#include <ctime>

struct timeval {
    time_t tv_sec; // seconds 
    long tv_usec; // microseconds
};

int gettimeofday(timeval* timeVal, void* ignore);

#endif