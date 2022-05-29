// Copyright 2022 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

// Mock implementation for unit testing (not targeting the ESP32)

// ReSharper disable CppInconsistentNaming - mimicking existing interface
// ReSharper disable CppParameterNeverUsed

// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppMemberFunctionMayBeStatic
// ReSharper disable CppParameterMayBeConst
#ifndef HEADER_ADAFRUIT_SSD1306_H
#define HEADER_ADAFRUIT_SSD1306_H

#include <Wire.h>
#include "../WaterMeterCpp/SafeCString.h"

constexpr uint16_t WHITE = 1;
constexpr uint16_t BLACK = 0;
constexpr uint8_t SSD1306_SWITCHCAPVCC = 0x02;

class Adafruit_SSD1306 {
public:
    Adafruit_SSD1306(unsigned width, unsigned height, TwoWire* wire) {}
    bool begin(uint8_t switchvcc, uint8_t i2caddr, bool reset = true, bool initDependencies = true) {
        return _isPresent;
    }
    void cp437(bool x = true) {}
    void display() {}
    void clearDisplay() {}
    void drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg) {
        _x = x;
        _y = y;
        _bitmap = bitmap;
        _w = w;
        _h = h;
        _fg = color;
        _bg = bg;
    }
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        _x = x;
        _y = y;
        _w = w;
        _h = h;
        _fg = color;
    }

    void setCursor(int16_t x, int16_t y) { _x = x; _y = y; }
    void setTextSize(uint8_t s) { _s = s; }
    void setTextColor(uint16_t c, uint16_t bg) { _fg = c; _bg = bg; }
    void print(const char* message) { safeStrcpy(_message, message); }

// testing only
    void sensorPresent(const bool isPresent) { _isPresent = isPresent; }
    int16_t getX() const { return _x; }
    int16_t getY() const { return _y; }
    int16_t getWidth() const { return _w; }
    int16_t getHeight() const { return _h; }
    int16_t getSize() const { return _s;  }
    uint16_t getForegroundColor() const { return _fg; }
    uint16_t getBackgroundColor() const { return _bg; }
    const char* getMessage() { return _message; }
    int getFirstByte() const { return *_bitmap; }

private:
    bool _isPresent = true;
    int16_t _y = 0;
    int16_t _x = 0;
    const uint8_t* _bitmap = nullptr;
    char _message[20] = "";
    int16_t _w = 0;
    int16_t _h = 0;
    int16_t _s = 0;
    uint16_t _fg = 0;
    uint16_t _bg = 0;
};
#endif
