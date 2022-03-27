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
// ReSharper disable CppParameterNeverUsed

#define WIN32_LEAN_AND_MEAN
#include <sys/time.h>
#include <windows.h>

int gettimeofday(timeval* timeVal, void* ignore) {
    if (timeVal) {
        FILETIME filetime; // 0.1 microsecond intervals since January 1, 1601 00:00 UTC 
        ULARGE_INTEGER x;
        ULONGLONG usec;
        static constexpr ULONGLONG EPOCH_OFFSET_MICROS = 11644473600000000ULL;
        // microseconds betweeen Jan 1,1601 and Jan 1,1970 

#if _WIN32_WINNT >= _WIN32_WINNT_WIN8
        GetSystemTimePreciseAsFileTime(&filetime);
#else
        GetSystemTimeAsFileTime(&filetime);
#endif
        x.LowPart = filetime.dwLowDateTime;
        x.HighPart = filetime.dwHighDateTime;
        usec = x.QuadPart / 10 - EPOCH_OFFSET_MICROS;
        timeVal->tv_sec = static_cast<time_t>(usec / 1000000ULL);
        timeVal->tv_usec = static_cast<long>(usec % 1000000ULL);
    }
    return 0;
}
