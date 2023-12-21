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

// Disabling warnings caused by mimicking existing interfaces
// ReSharper disable CppInconsistentNaming
// ReSharper disable CppMemberFunctionMayBeStatic

#ifndef HEADER_PREFERENCES
#define HEADER_PREFERENCES
#include <map>

#include "StringArduino.h"

/**
 * \brief Mock implementation of the Preferences driver for unit testing (not targeting the ESP32)
 */
class Preferences {
public:
    bool begin(const char* name, bool readOnly = false);
    void clear();
    void end();
    bool getBool(const char* key, bool defaultValue);
    void getBytes(const char* key, void* buf, size_t maxLen);
    String getString(const char* key);
    unsigned getUInt(const char* key, int defaultValue);
    bool isKey(const char* key);
    void putString(const char* key, const char* value);
    void putBool(const char* key, bool value);
    void putBytes(const char* key, const void* value, size_t len);
    void putUInt(const char* key, uint32_t value);
    void save();
    void load();
private:
    std::map<std::string, std::map<std::string, std::string>> _preferences;

    std::map<std::string, std::string>* _currentPreference = nullptr;
    bool _started = false;
};

#endif
