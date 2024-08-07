// Copyright 2021-2024 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#pragma once
#include "FirmwareManager.h"

// For testing we make two protected methods publicly available. Otherwise, it should behave the same as FirmwareManager.
namespace WaterMeterCppTest {
	using WaterMeter::FirmwareManager;

	class FirmwareManagerDriver final : public FirmwareManager {
	public:
		using FirmwareManager::FirmwareManager;
		using FirmwareManager::loadUpdate;
		using FirmwareManager::isUpdateAvailable;
	};
}
