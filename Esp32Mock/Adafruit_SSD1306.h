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

// ReSharper disable CppInconsistentNaming - mimicking existing interface
// ReSharper disable CppParameterNeverUsed

// ReSharper disable CppMemberFunctionMayBeConst
// ReSharper disable CppMemberFunctionMayBeStatic
// ReSharper disable CppParameterMayBeConst
#ifndef HEADER_ADAFRUIT_SSD1306_H
#define HEADER_ADAFRUIT_SSD1306_H

#include <Wire.h>
#include <SafeCString.h>

constexpr uint16_t WHITE = 1;
constexpr uint16_t BLACK = 0;
constexpr uint8_t SSD1306_SWITCHCAPVCC = 0x02;

/**
 * \brief Mock implementation of the SSD1306 OLED display driver for unit testing (not targeting the ESP32)
 */
class Adafruit_SSD1306 {
public:
    Adafruit_SSD1306(unsigned width, unsigned height, TwoWire* wire) {}

    /**
     * \brief 
     * \param switchvcc VCC selection. Typically SSD1306_SWITCHCAPVCC
     * \param i2caddr I2C address. Typically 0x3C
     * \param reset whether to perform a hard reset before initializing
     * \param initDependencies call begin on the dependencies (Wire) as well
     * \return true on success, false on failure
     */
    bool begin(uint8_t switchvcc, uint8_t i2caddr, bool reset = true, bool initDependencies = true) {
        return _isPresent;
    }

    void cp437(bool x = true) {}

    /**
     * \brief push the display buffer to the hardware
     */
    void display() {}

    /**
     * \brief Clear the display buffer
     */
    void clearDisplay() {}

    /**
     * \brief Draw a bitmap
     * \param x x coordinate of the top left corner
     * \param y y coordinate of the top left corner
     * \param bitmap bitmap to draw
     * \param w width of the bitmap
     * \param h height of the bitmap
     * \param color foreground color
     * \param bg background color
     */
    void drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg) {
        _x = x;
        _y = y;
        _bitmap = bitmap;
        _w = w;
        _h = h;
        _fg = color;
        _bg = bg;
    }

    /**
     * \brief Fill a rectangle
     * \param x x coordinate of the top left corner
     * \param y y coordinate of the top left corner
     * \param w width of the rectangle
     * \param h height of the rectangle
     * \param color foreground color
     */
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        _x = x;
        _y = y;
        _w = w;
        _h = h;
        _fg = color;
    }

    /**
     * \brief Set the cursor position
     * \param x x coordinate to set
     * \param y y coordinate to set
     */
    void setCursor(int16_t x, int16_t y) {
        _x = x;
        _y = y;
    }

    /**
     * \brief Set text size
     * \param s Size to set
     */
    void setTextSize(uint8_t s) { _s = s; }

    /**
     * \brief set foreground and background color
     * \param c foreground color
     * \param bg background color
     */
    void setTextColor(uint16_t c, uint16_t bg) {
        _fg = c;
        _bg = bg;
    }

    /**
     * \brief print a message
     * \param message message to print
     */
    void print(const char* message) { SafeCString::strcpy(_message, message); }

    // testing only
    /**
     * \brief Testing
     * \param isPresent whether the sensor is present
     */
    void sensorPresent(const bool isPresent) { _isPresent = isPresent; }

    /**
     * \brief Testing
     * \return X coordinate of last draw operation
     */
    int16_t getX() const { return _x; }

    /**
     * \brief Testing
     * \return Y coordinate of last draw operation
     */
    int16_t getY() const { return _y; }

    /**
     * \brief Testing
     * \return Width of last draw operation
     */
    int16_t getWidth() const { return _w; }

    /**
     * \brief Testing
     * \return Height of last draw operation
     */
    int16_t getHeight() const { return _h; }

    /**
     * \brief Testing
     * \return Size of last draw operation
     */
    int16_t getSize() const { return _s; }

    /**
     * \brief Testing
     * \return Foreground color of last draw operation
     */
    uint16_t getForegroundColor() const { return _fg; }

    /**
     * \brief Testing
     * \return Background color of last draw operation
     */
    uint16_t getBackgroundColor() const { return _bg; }

    /**
     * \brief Testing
     * \return Last message printed
     */
    const char* getMessage() { return _message; }

    /**
     * \brief Testing
     * \return First byte of last bitmap drawn
     */
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
