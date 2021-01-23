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

#ifndef HEADER_SERIALDRIVER
#define HEADER_SERIALDRIVER

class SerialDriver {
public:
	SerialDriver();
	void begin();
	char* getCommand();
	char* getParameter();
	bool inputAvailable();
	void println(const char *line);
	void clearInput();
protected:
	// 115200 is a speed that a Pi 2 should be able to read comfortably. Can't go much slower to keep up.
    static const long SERIAL_SPEED = 115200;
	static const int INPUT_BUFFER_SIZE = 50;
	char _buffer[INPUT_BUFFER_SIZE];
	int _commandIndex = 0;
	bool _initialized = false;
	bool _inputAvailable = false;
	char* _parameter = 0;
};

#endif
