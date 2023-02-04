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

// Mock implementation of HTTPUpdate for unit testing (not targeting the ESP32)

// Disabling warnings caused by mimicking existing interface
// ReSharper disable CppMemberFunctionMayBeStatic
// ReSharper disable CppInconsistentNaming
// ReSharper disable CppParameterNeverUsed
// ReSharper disable CppParameterMayBeConst
// ReSharper disable CppClangTidyPerformanceUnnecessaryValueParam

#ifndef HEADER_HTTPUPDATE
#define HEADER_HTTPUPDATE

#include <functional>

#include "StringArduino.h"
#include "WiFiClient.h"

constexpr int HTTP_UPDATE_FAILED = 0;
constexpr int HTTP_UPDATE_NO_UPDATES = 1;
constexpr int HTTP_UPDATE_OK = 2;

using t_httpUpdate_return = int;
using HTTPUpdateStartCB = std::function<void()>;
using HTTPUpdateEndCB = std::function<void()>;
using HTTPUpdateErrorCB = std::function<void(int)>;
using HTTPUpdateProgressCB = std::function<void(int, int)>;

class HTTPUpdate {
public:
    t_httpUpdate_return update(WiFiClient& client, const char* url);

    int getLastError() { return 0; }
    String getLastErrorString() { return {"OK"}; }
    static int ReturnValue;

    void onStart(HTTPUpdateStartCB cbOnStart) { _cbStart = cbOnStart; }
    void onEnd(HTTPUpdateEndCB cbOnEnd) { _cbEnd = cbOnEnd; }
    void onError(HTTPUpdateErrorCB cbOnError) { _cbError = cbOnError; }
    void onProgress(HTTPUpdateProgressCB cbOnProgress) { _cbProgress = cbOnProgress; }
private:
    HTTPUpdateStartCB    _cbStart;
    HTTPUpdateEndCB      _cbEnd;
    HTTPUpdateErrorCB    _cbError;
    HTTPUpdateProgressCB _cbProgress;
};

extern HTTPUpdate httpUpdate;

#endif
