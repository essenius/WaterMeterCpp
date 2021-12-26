#pragma once
#include "pch.h"
#include "CppUnitTest.h"
#include <string>
#include "../WaterMeterCpp/PubSub.h"

namespace Microsoft {
    namespace VisualStudio {
        namespace CppUnitTestFramework {

            template<>
            static std::wstring ToString<Topic>(const Topic& topic) {
                return std::to_wstring(static_cast<int>(topic));
            }
        }
    }
}
