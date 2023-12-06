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

// Disabling warnings caused by mimicking existing interface
// ReSharper disable CppInconsistentNaming

#ifndef WIRE_HEADER
#define WIRE_HEADER

#include "Wire.h"
#include "ESP.h"

/**
 * \brief Mock implementation of the Wire protocol driver for unit testing (not targeting the ESP32)
 */
class TwoWire {
public:
    /**
     * \brief 
     * \return number of bytes available for reading 
     */
    int available();

    /**
     * \brief Initialize the Wire library with a given I2C address and pins
     * \param sda the pin to use for SDA (data)
     * \param scl the pin to use for SCL (clock)
     */
    void begin(int sda = 21, int scl = 22);

    /**
     * \brief begin transmission to a given address
     * \param address the 7-bit address of the device to transmit to
     */
    void beginTransmission(uint8_t address);

    /**
     * \brief read a byte from the I2C bus
     * \return byte read from the I2C bus
     */
    int read();

    size_t write(uint8_t value);

    uint8_t endTransmission();
    /**
     * \brief request a given number of bytes from a given address
     * \param address the 7-bit address of the device to request bytes from
     * \param size the number of bytes to request
     * \param stop enable or disable the stop bit after the request
     * \return the number of bytes returned
     */
     uint8_t requestFrom(uint8_t address, uint8_t size, bool stop);

    /**
     * \brief Test method
     * \return the configured address
     */
    uint8_t getAddress();

    /**
     * \brief Test method: set the toggle period for the EndTransmission result to test e.g. sensor.isOn
     * \param period the number of times endTransmission is called before the result toggles
     */
    void setEndTransmissionTogglePeriod(int period);

    // testing - force reads to return the same value

    /**
     * \brief Test method: set whether to return the same value going forward, and the value itself
     * \param flatline 
     * \param value 
     */
    void setFlatline(bool flatline, uint8_t value = 0);

    // testing - return -1 if length wrong, index of wrong item if content wrong, or length if all OK.

    /**
     * \brief Test method: find out if we have a mismatch
     * \param expected the expected values
     * \param length the length of the expected values
     * \return return -1 if length wrong, index of wrong item if content wrong, or length if all OK
     */
    short writeMismatchIndex(const uint8_t* expected, short length) const;

private:
    static constexpr short WriteBufferSize = 2048;
    uint8_t _address = 0;
    int _available = 0;
    uint8_t _nextResult = 0;
    uint8_t _written[WriteBufferSize]{};
    short _writeIndex = 0;
    bool _flatline = false;
    int _endTransmissionCounter = 10;
    uint8_t _endTransmissionValue = 0;
    int _endTransmissionTogglePeriod = 10;
};

extern TwoWire Wire;
extern TwoWire Wire1;
#endif
