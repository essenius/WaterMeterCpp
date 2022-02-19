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

// Mock implementation for if we are not targeting the Arduino

// ReSharper disable CppMemberFunctionMayBeStatic -- need to mimic existing interface
// ReSharper disable CppMemberFunctionMayBeConst -- same here

#ifndef ESP32

#ifndef HEADER_QMC5883LCOMPASS
#define HEADER_QMC5883LCOMPASS

class QMC5883LCompass {
public:
    // mock existing interface
    void init() {}
    void read() {}
    void setCalibration(int a, int b, int c, int d, int e, int f) {}

    void setReset() {
        _resetCount++;
        if (_resetSucceeds) _y++;
    }

    int getY() { return _y; }
    void setMode(int i, int i1, int i2, int i3) {}

    // testing purposes
    int resetCount() { return _resetCount; }
    void resetSucceeds(const bool succeeds) { _resetSucceeds = succeeds; }
    void setY(const int16_t value) { _y = value; }
private:
    int _resetCount = 0;
    bool _resetSucceeds = true;
    int _y = 0;
};
#endif

#endif
