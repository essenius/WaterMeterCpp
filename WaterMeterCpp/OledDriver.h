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

#ifndef HEADER_OLEDDRIVER
#define HEADER_OLEDDRIVER
#include <Adafruit_SSD1306.h>

#include "EventServer.h"

class OledDriver final : public EventClient {
public:
    explicit OledDriver(EventServer* eventServer);
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
    void switchFlowLogo(const unsigned char* logo, long switchOn);
    void switchLogo(const unsigned char* logo, int16_t xLocation, int16_t yLocation, long switchOn);    
    
    Adafruit_SSD1306 _display;
    bool _needsDisplay = false;
    static constexpr unsigned int MIN_DELAY_FOR_DISPLAY = 25;
    static constexpr unsigned int SCREEN_WIDTH = 128;
    static constexpr unsigned int SCREEN_HEIGHT = 32;
    static constexpr unsigned int LOGO_HEIGHT = 7;
    static constexpr unsigned int LOGO_WIDTH = 8;
    static constexpr unsigned int FLOW_X = 98;
    static constexpr unsigned int FLOW_Y = 0;
    static constexpr unsigned int EVENT_X = 108;
    static constexpr unsigned int EVENT_Y = 0;
    static constexpr unsigned int CONNECTION_X = 118;
    static constexpr unsigned int CONNECTION_Y = 0;
            
    static constexpr unsigned char MQTT_LOGO[] =
    { 0b00000000, 
      0b11100000, 
      0b00010000,
      0b11001000,
      0b00100100,
      0b10010100,
      0b11010100 };
    };
    
    static constexpr unsigned char WIFI_LOGO[] =
    { 0b01111000, 
      0b10000100, 
      0b00000000,
      0b01111000, 
      0b10000100, 
      0b00110000,
      0b00110000 };
    
    
    static constexpr unsigned char TIME_LOGO[] =
    { 0b00110000, 
      0b01101000, 
      0b10100100, 
      0b10111100,
      0b10000100, 
      0b01001000, 
      0b00110000 };
    
    
    static constexpr unsigned char DOWNLOAD_LOGO[] =
    { 0b00110000, 
      0b00110000, 
      0b00110000, 
      0b00110000, 
      0b11111100, 
      0b01111000, 
      0b00110000 };
    
    static constexpr unsigned char SEND_LOGO[] =
    { 0b11111100, 
      0b10000100, 
      0b11001100, 
      0b10110100, 
      0b10000100, 
      0b10000100, 
      0b11111100 };
    
    static constexpr unsigned char NO_SENSOR_LOGO[] =
    { 0b00010011, 
      0b00010110, 
      0b00111100, 
      0b11011011, 
      0b00110100, 
      0b01111000, 
      0b11010000 };
    
    
    static constexpr unsigned char FLOW_LOGO[] =
    { 0b00010000, 
      0b10101010, 
      0b01000100, 
      0b00010000, 
      0b10101010, 
      0b01000100, 
      0b00000000
     };
    
    static constexpr unsigned char ALERT_LOGO[] =
    { 0b11111100, 
      0b01111000, 
      0b01111000, 
      0b00110000, 
      0b00000000, 
      0b00110000, 
      0b00110000
     };
    
    static constexpr unsigned char BLOCKED_LOGO[] =
    { 0b00111000, 
      0b01111100, 
      0b11111110, 
      0b10000010, 
      0b11111110, 
      0b01111100, 
      0b00111000
     };
    
    static constexpr unsigned char RESET_LOGO[] =
    { 0b00010000, 
      0b01010100, 
      0b10010010, 
      0b10010010, 
      0b10000010, 
      0b01000100, 
      0b00111000
     };
    
    static constexpr unsigned char OUTLIER_LOGO[] =
    { 0b00011000, 
      0b00011000, 
      0b00000000, 
      0b11111111, 
      0b00000000, 
      0b00000000, 
      0b11111111
     };
    static constexpr unsigned char FIRMWARE_LOGO[] = { 
      0b11110001, 
      0b10000001, 
      0b11110001, 
      0b10100001, 
      0b10101001, 
      0b10101010, 
      0b10010100
    };
#endif
