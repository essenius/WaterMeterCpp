#pragma once

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

#pragma once
#include "pch.h"
#include "CppUnitTest.h"
#include <string>
#include "../WaterMeterCpp/Connection.h"

enum class ConnectionState;

namespace Microsoft {
    namespace VisualStudio {
        namespace CppUnitTestFramework {

            template <> 
            static std::wstring ToString<ConnectionState>(const ConnectionState& state) {
                return std::to_wstring(static_cast<int>(state));
            }
        }
    }
}
