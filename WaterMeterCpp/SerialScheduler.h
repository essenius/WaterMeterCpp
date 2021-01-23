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

#ifndef HEADER_SERIALSCHEDULER
#define HEADER_SERIALSCHEDULER

#include "SerialDriver.h"
#include "BatchWriter.h"

class SerialScheduler
{
public:
	// Disconnect if a message is not acknowledged within 250 ms
	static const long RESPONSE_TIMEOUT_MICROS = 250L * 1000L;
	SerialScheduler(SerialDriver* serialDriver, BatchWriter* measureWriter, BatchWriter* resultWriter, BatchWriter* infoWriter, int (* freeMemory)());
	bool isConnected();
	int optimalWriteCount(int round);
	bool processInput();
	bool processOutput();
	void setCanFlush();
	bool hasDelayedFlush();
	bool wasResultWritten();
	void writeHeaders();
private:
	static const int MEASURE = 0;
	static const int RESULT = 1;
	static const int INFO = 2;
	static const int WRITER_COUNT = 3;
	SerialDriver* _serialDriver;
	BatchWriter* _writer[WRITER_COUNT];
	bool _delayedFlush = false;
	bool _isConnected = false;
	bool _resultWritten = false;
	long _lastSent = 0L;
	int (* _freeMemory)();
};

#endif