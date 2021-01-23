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

#include "SerialDriver.h"
#include <string.h>

SerialDriver::SerialDriver() {
    _buffer[0] = 0;
}

void SerialDriver::begin() {
    Serial.begin(SERIAL_SPEED);
    Serial.setTimeout(10);
    _initialized = true;
}

char* SerialDriver::getCommand() {
    if (!_initialized || !_inputAvailable) {
        return 0;
    }
    if (_parameter != 0) {
        return _buffer;
    }
    char* location = strchr(_buffer, ' ');
    if (location != 0) {
      _parameter = location + 1;
      *location = 0;
    } else {
      _parameter = 0;
    }
    return _buffer;
}

char* SerialDriver::getParameter() {
    if (_parameter != 0) {
        return _parameter;
    }
    return (getCommand() == 0 || _parameter == 0) ? 0 : _parameter;
}

void SerialDriver::clearInput() {
    _buffer[0] = 0;
    _commandIndex = 0;
    _parameter = 0;
}

bool SerialDriver::inputAvailable() {
    if (!_initialized) {
      return false;
    }
    _inputAvailable = false;

    while (Serial.available() > 0 && !_inputAvailable) {
        char characterIn = Serial.read();
        if (characterIn == '\n' || characterIn == '|') {
            _inputAvailable = true;
        }
        else {
            if (characterIn >= 'a' && characterIn <= 'z') characterIn += 'A' - 'a';
            _buffer[_commandIndex++] = characterIn;
            _buffer[_commandIndex] = 0;
            _inputAvailable = _commandIndex >= INPUT_BUFFER_SIZE - 1;
        }
    }
    return _inputAvailable; 
}

void SerialDriver::println(const char* line) {
    if (_initialized) {
        Serial.println(line);
    }
}
