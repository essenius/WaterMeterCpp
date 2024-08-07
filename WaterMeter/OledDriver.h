// Copyright 2022-2024 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

// drives an I2C 128x32 OLED display. Shows icons to indicate status/errors, shows the pulses and volumes, and
// potentially short error messages.

#ifndef HEADER_OLED_DRIVER
#define HEADER_OLED_DRIVER
#include <Adafruit_SSD1306.h>

#include "ConnectionState.h"
#include "EventServer.h"

namespace WaterMeter {
    class OledDriver final : public EventClient {
    public:
        explicit OledDriver(EventServer* eventServer, TwoWire* wire = &Wire);
        Adafruit_SSD1306* getDriver();
        bool begin();
        unsigned int display();
        void update(Topic topic, const char* payload) override;
        void update(Topic topic, long payload) override;

    private:
        void clearLogo(int16_t xLocation, int16_t yLocation);
        void clearConnectionLogo();
        void drawBitmap(const unsigned char* logo, int16_t xLocation, int16_t yLocation);
        void setConnectionLogo(const unsigned char* logo);
        void setLogo(const unsigned char* logo, int16_t xLocation, int16_t yLocation);
        void showMessageAtLine(const char* message, int16_t line);
        void switchEventLogo(const unsigned char* logo, long switchOn);
        // TODO: replace by pulseLogo
        void switchFlowLogo(const unsigned char* logo, long switchOn);
        void switchLogo(const unsigned char* logo, int16_t xLocation, int16_t yLocation, long switchOn);
        void updateConnectionState(ConnectionState payload);

        TwoWire* _wire;
        Adafruit_SSD1306 _display;
        bool _needsDisplay = false;
        static constexpr unsigned int MinDelayForDisplay = 25;
        static constexpr int16_t ScreenWidth = 128;
        static constexpr int16_t ScreenHeight = 32;
        static constexpr int16_t LineHeight = 8;
        static constexpr int16_t LogoHeight = 7;
        static constexpr int16_t LogoWidth = 8;
        static constexpr int16_t FlowX = 98;
        static constexpr int16_t FlowY = 0;
        static constexpr int16_t EventX = 108;
        static constexpr int16_t EventY = 0;
        static constexpr int16_t ConnectionX = 118;
        static constexpr int16_t ConnectionY = 0;

        static constexpr unsigned char AlertLogo[] =
        {
            0b11111100,
            0b01111000,
            0b01111000,
            0b00110000,
            0b00000000,
            0b00110000,
            0b00110000
        };

        static constexpr unsigned char BlockedLogo[] =
        {
            0b00111000,
            0b01111100,
            0b11111110,
            0b10000010,
            0b11111110,
            0b01111100,
            0b00111000
        };

        static constexpr unsigned char DownloadLogo[] =
        {
            0b00110000,
            0b00110000,
            0b00110000,
            0b00110000,
            0b11111100,
            0b01111000,
            0b00110000
        };

        static constexpr unsigned char FirmwareLogo[] =
        {
            0b11110001,
            0b10000001,
            0b11110001,
            0b10100001,
            0b10101001,
            0b10101010,
            0b10010100
        };

        static constexpr unsigned char MqttLogo[] =
        {
            0b00000000,
            0b11100000,
            0b00010000,
            0b11001000,
            0b00100100,
            0b10010100,
            0b11010100
        };

        static constexpr unsigned char NoFitLogo[] =
        {
            0b00111001,
            0b01000110,
            0b10001101,
            0b10011001,
            0b10110001,
            0b01100100,
            0b10111000
        };

        static constexpr unsigned char NoSensorLogo[] =
        {
            0b00010011,
            0b00010110,
            0b00111100,
            0b11011011,
            0b00110100,
            0b01111000,
            0b11010000
        };

        static constexpr unsigned char OutlierLogo[] =
        {
            0b00011000,
            0b00011000,
            0b00000000,
            0b11111111,
            0b00000000,
            0b00000000,
            0b11111111
        };

        static constexpr unsigned char ResetLogo[] =
        {
            0b00010000,
            0b01010100,
            0b10010010,
            0b10010010,
            0b10000010,
            0b01000100,
            0b00111000
        };

        static constexpr unsigned char SendLogo[] =
        {
            0b11111100,
            0b10000100,
            0b11001100,
            0b10110100,
            0b10000100,
            0b10000100,
            0b11111100
        };

        static constexpr unsigned char TimeLogo[] =
        {
            0b00110000,
            0b01101000,
            0b10100100,
            0b10111100,
            0b10000100,
            0b01001000,
            0b00110000
        };

        static constexpr unsigned char WifiLogo[] =
        {
            0b01111000,
            0b10000100,
            0b00000000,
            0b01111000,
            0b10000100,
            0b00110000,
            0b00110000
        };

    };
}
#endif
