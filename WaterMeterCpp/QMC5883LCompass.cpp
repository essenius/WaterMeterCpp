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

#ifndef ARDUINO

	#include "QMC5883LCompass.h"

	void QMC5883LCompass::init() {
	}

	void QMC5883LCompass::read() {
	}

	void QMC5883LCompass::setCalibration(int a, int b, int c, int d, int e, int f) {
	}

	int QMC5883LCompass::getY() {
		return 0;
	}

#endif