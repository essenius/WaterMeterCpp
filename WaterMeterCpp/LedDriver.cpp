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

#ifdef ARDUINO
  #include <Arduino.h>
#else 
  #include "Arduino.h"
#endif

#include "LedDriver.h"
#include <stdint.h>

void LedDriver::begin() {
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(RED_LED, OUTPUT);
    pinMode(GREEN_LED, OUTPUT);
    pinMode(BLUE_LED, OUTPUT);
    toggleBuiltin();
    digitalWrite(RED_LED, 0);
    digitalWrite(GREEN_LED, 0);
    digitalWrite(BLUE_LED, 0);
}

void LedDriver::signalFlush(bool hasFlushed) {
    digitalWrite(BLUE_LED, hasFlushed);
}

void LedDriver::signalConnected(bool isConnected) {
    digitalWrite(GREEN_LED, isConnected);
}

void LedDriver::signalError(bool hasError) {
    digitalWrite(RED_LED, hasError);  
}

void LedDriver::signalInput(bool hasInput) {
    digitalWrite(RED_LED, hasInput);
}

void LedDriver::signalMeasurement(bool isExcluded, bool hasFlow) {
    unsigned int ledInterval = isExcluded ? EXCLUDE_INTERVAL : hasFlow ? FLOW_INTERVAL : WAIT_INTERVAL;

    if (_interval != ledInterval) {
        _ledCounter = ledInterval;
        _interval = ledInterval;
    }
    if (_ledCounter == 0) {
        toggleBuiltin();
        _ledCounter = ledInterval;
    }
    _ledCounter--;
}


void LedDriver::toggleBuiltin() {
    _isOn = !_isOn;
    digitalWrite(LED_BUILTIN, _isOn);
}
