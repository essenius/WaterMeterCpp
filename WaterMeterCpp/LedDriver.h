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
#ifndef HEADER_LEDDRIVER
#define HEADER_LEDDRIVER

class LedDriver {
public:
	void begin();
	void toggleBuiltin();
	void signalError(bool hasError);
    void signalFlush(bool hasFlushed);
	void signalInput(bool hasError);
	void signalMeasurement(bool isExcluded, bool hasFlow);
    void signalConnected(bool isConnected);
  #ifdef ESP8266
  static const unsigned char BLUE_LED = D5;
  static const unsigned char GREEN_LED = D6;
  static const unsigned char RED_LED = D7;
  #else
	static const unsigned char BLUE_LED = 2;
	static const unsigned char GREEN_LED = 3;
	static const unsigned char RED_LED = 4;
 #endif
	static const unsigned int WAIT_INTERVAL = 100;
	static const unsigned int FLOW_INTERVAL = 50;
	static const unsigned int EXCLUDE_INTERVAL = 25;
private:
	unsigned int _interval = 0;
	unsigned int _ledCounter = 0;
	bool _isOn = false;
};

#endif
