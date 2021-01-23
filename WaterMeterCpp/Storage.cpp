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

#include "Storage.h"
#include <string.h>

#ifdef ARDUINO
  #include <EEPROM.h>
#else
  #include <iostream>
  #include <fstream>
#endif

const int Storage::STORAGE_SIZE = sizeof(Rates);

Storage::Storage() {
    load();
}

// On Arduino we use EEPROM. On other devices we use a file based storage
#ifdef ARDUINO
void Storage::clear() {
    for (int i = 0; i <= STORAGE_SIZE; i++) {
        EEPROM.write(i, 0xFF);
    }
    load();
}

void Storage::dump(char* buffer) {
    for (int i = 0; i <= STORAGE_SIZE; i++) {
        buffer[i] = EEPROM.read(i);
    }
}

void Storage::load() {
    EEPROM.get(0, _rates);
    _signatureOk = strcmp(_rates.signature, STORAGE_SIGNATURE) == 0;
}

void Storage::save() {
    if (!_signatureOk) {
        strcpy(_rates.signature, STORAGE_SIGNATURE);
        _signatureOk = true;
    }
    EEPROM.put(0, _rates);
}

#else

const char FILENAME[] = "storage.txt";

Storage::Storage(bool clearIt) {
    if (clearIt) {
        clear();
    }
    load();
}

void Storage::clear() {
    remove(FILENAME);
    _rates.idleRate = -1L;
    _rates.nonIdleRate = -1L;
    _rates.logRate = 0xFF;
}

void Storage::dump(char* buffer) {
    char numberBuffer[15] = { 0 };
    strcpy_s(buffer, STORAGE_SIGNATURE_SIZE, _rates.signature);
    strcat_s(buffer, 80, ",");
    _ltoa_s(_rates.idleRate, numberBuffer, 15, 10);
    strcat_s(buffer, 80, numberBuffer);
    strcat_s(buffer, 80, ",");
    _ltoa_s(_rates.nonIdleRate, numberBuffer, 15, 10);
    strcat_s(buffer, 80, numberBuffer);
    strcat_s(buffer, 80, ",");
    _ltoa_s(_rates.logRate, numberBuffer, 15, 10);
    strcat_s(buffer, 80, numberBuffer);
}

void Storage::load() {
    std::ifstream storageFile(FILENAME);
    char line[80];
    storageFile.getline(line, 80);
    storageFile.close();
    if (strlen(line) > 0) {
        rsize_t strmax = sizeof line;
        char* nextToken = 0;
        char* token = strtok_s(line, ",", &nextToken);
        if (token == 0) return;
        strncpy_s(_rates.signature, token, STORAGE_SIGNATURE_SIZE);
        token = strtok_s(0, ",", &nextToken);
        if (token == 0) return;
        _rates.idleRate = atol(token);
        token = strtok_s(0, ",", &nextToken);
        if (token == 0) return;
        _rates.nonIdleRate = atol(token);
        token = strtok_s(0, ",", &nextToken);
        if (token == 0) return;
        _rates.logRate = atol(token);
    }
    _signatureOk = strcmp(_rates.signature, STORAGE_SIGNATURE) == 0;
}

void Storage::save() {
    if (!_signatureOk) {
        strcpy_s(_rates.signature, STORAGE_SIGNATURE_SIZE, STORAGE_SIGNATURE);
        _signatureOk = true;
    }
    std::ofstream storageFile(FILENAME);
    char line[80];
    dump(line);
    storageFile << line << std::endl;
    storageFile.close();
}

#endif

long Storage::getIdleRate() {
    return _signatureOk ? _rates.idleRate : -1L;
}

long Storage::getIdleRate(long defaultRate) {
    if (getIdleRate() == -1L) {
        setIdleRate(defaultRate);
    }
    return getIdleRate();
}

unsigned char Storage::getLogRate() {
    return _signatureOk ? _rates.logRate : 0xFF;
}

unsigned char Storage::getLogRate(unsigned char defaultRate) {
    if (getLogRate() == 0xFF) {
        setLogRate(defaultRate);
    }
    return getLogRate();
}

long Storage::getNonIdleRate() {
    return _signatureOk ? _rates.nonIdleRate : -1L;
}

long Storage::getNonIdleRate(long defaultRate) {
    if (getNonIdleRate() == -1L) {
        setNonIdleRate(defaultRate);
    }
    return getNonIdleRate();
}

void Storage::setIdleRate(long rate) {
    _rates.idleRate = rate;
    save();
}

void Storage::setLogRate(unsigned char rate) {
    _rates.logRate = rate;
    save();
}

void Storage::setNonIdleRate(long rate) {
    _rates.nonIdleRate = rate;
    save();
}
