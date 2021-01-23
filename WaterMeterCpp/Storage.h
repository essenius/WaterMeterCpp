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

#ifndef HEADER_STORAGE
#define HEADER_STORAGE

const char STORAGE_SIGNATURE[] = "WTM01";
const char STORAGE_SIGNATURE_SIZE = sizeof(STORAGE_SIGNATURE);

struct Rates {
  char signature[STORAGE_SIGNATURE_SIZE];
  long idleRate = -1L;
  long nonIdleRate = -1L;
  unsigned char logRate = 0xFF;
};

class Storage {
public:
	Storage();
#ifndef ARDUINO
	Storage(bool clearIt);
#endif
	void clear();
	void dump(char* buffer);
	long getIdleRate();
	long getIdleRate(long defaultRate);
	unsigned char getLogRate();
	unsigned char getLogRate(unsigned char defaultRate);
	long getNonIdleRate();
    long getNonIdleRate(long defaultRate);
	void setIdleRate(long rate);
    void setLogRate(unsigned char rate);
	void setNonIdleRate(long rate);
    static const int STORAGE_SIZE;
private:
    void load();
    void save();
	bool _signatureOk;
    Rates _rates;
};

#endif
