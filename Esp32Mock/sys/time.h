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

// Mock implementation of the timeval struct and gettimeofday function for unit testing (not targeting the ESP32)

// Disabling warnings caused by mimicking existing interface
// ReSharper disable CppInconsistentNaming
// ReSharper disable CppNonInlineFunctionDefinitionInHeaderFile

#ifndef HEADER_TIME
#define HEADER_TIME
#include <ctime>

/**
 * \brief Mock implementation of the timeval struct for unit testing (not targeting the ESP32)
 */
struct timeval {
    time_t tv_sec; // seconds 
    long tv_usec; // microseconds
};

/**
 * \brief Mock implementation of the gettimeofday function for unit testing (not targeting the ESP32)
 * \param timeVal the time value returned
 * \param ignore an ignored value
 * \return 0 on success, -1 on error
 */
int gettimeofday(timeval* timeVal, void* ignore);

#endif
