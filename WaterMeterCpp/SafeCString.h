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

#ifndef HEADER_SAFECSTRING
#define HEADER_SAFECSTRING

#include <cstring>

template <size_t BufferSize>
void safeStrcpy(char (&buffer)[BufferSize], const char* source) {
    strncpy(buffer, source, BufferSize);
    buffer[BufferSize - 1] = 0;
}

template <size_t BufferSize>
void safePointerStrcpy(char* const pointer, char(&buffer)[BufferSize], const char* source) {
    auto charactersUsed = pointer - buffer;
    // if the pointer is not in the right range, do nothing
    if (charactersUsed < 0 || charactersUsed > BufferSize) return;
    strncpy(pointer, source, BufferSize - charactersUsed - 1);
    buffer[BufferSize - 1] = 0;
}

template <size_t BufferSize, typename... Arguments>
void safeSprintf(char (&buffer)[BufferSize], const char* format,  Arguments... arguments) {
    snprintf(buffer, BufferSize, format, arguments...);
}

template <size_t BufferSize, typename... Arguments>
void safePointerSprintf(char* const pointer, char(&buffer)[BufferSize], const char* format, Arguments... arguments) {
    auto charactersUsed = pointer - buffer;
    if (charactersUsed < 0 || charactersUsed > BufferSize) return;
    snprintf(pointer, BufferSize - charactersUsed, format, arguments...);
}

template <size_t BufferSize>
void safeStrcat(char(&buffer)[BufferSize], const char* source) {
    strncat(buffer, source, BufferSize - strlen(buffer) - 1);
    buffer[BufferSize - 1] = 0;
}

#endif
