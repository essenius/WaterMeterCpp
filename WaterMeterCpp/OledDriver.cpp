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

#include <Wire.h>
#include "OledDriver.h"
#include "ConnectionState.h"
#include "SafeCString.h"

OledDriver::OledDriver(EventServer* eventServer) :
    EventClient(eventServer),
    _display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire) {}
    

// for testing only
Adafruit_SSD1306* OledDriver::getDriver() { return &_display; }

bool OledDriver::begin() {
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    constexpr int OLED_128_32 = 0x3c;
    if(!_display.begin(SSD1306_SWITCHCAPVCC, OLED_128_32, false)) {
      _eventServer->publish(Topic::NoDisplayFound, LONG_TRUE);
      return false;
    }
    _display.clearDisplay();
    _display.setTextSize(1);      
    _display.setTextColor(WHITE, BLACK); // white text on black background
    _display.cp437(true);                // Use full 256 char 'Code Page 437' font
    showMessageAtLine("Starting", 3);
    _display.display();
    _needsDisplay = false;
    _eventServer->subscribe(this, Topic::Alert);
    _eventServer->subscribe(this, Topic::Blocked);
    _eventServer->subscribe(this, Topic::Connection);
    _eventServer->subscribe(this, Topic::Flow);
    _eventServer->subscribe(this, Topic::NoSensorFound);
    _eventServer->subscribe(this, Topic::SensorWasReset);
    _eventServer->subscribe(this, Topic::TimeOverrun);
    _eventServer->subscribe(this, Topic::Meter);
    _eventServer->subscribe(this, Topic::Peaks);
    delay(MIN_DELAY_FOR_DISPLAY);
    return true;
}

void OledDriver::clearConnectionLogo() {
    clearLogo(118,0);
}

void OledDriver::clearLogo(const int16_t xLocation, const int16_t yLocation) {
    _display.fillRect(xLocation, yLocation, LOGO_WIDTH, LOGO_HEIGHT, BLACK);
    _needsDisplay = false;
}

unsigned int OledDriver::display() {
  if (_needsDisplay) {
      _display.display();
      _needsDisplay = false;
      delay(MIN_DELAY_FOR_DISPLAY);
      return MIN_DELAY_FOR_DISPLAY;
  }
  return 0;
}

void OledDriver::drawBitmap(const unsigned char* logo, const int16_t xLocation, const int16_t yLocation) {
    _display.drawBitmap(xLocation, yLocation, logo, LOGO_WIDTH, LOGO_HEIGHT, WHITE, BLACK);
    _needsDisplay = true;
}

void OledDriver::setConnectionLogo(const unsigned char* logo) {
    setLogo(logo, CONNECTION_X, CONNECTION_Y);  
}

void OledDriver::setLogo(const unsigned char* logo, const int16_t xLocation, const int16_t yLocation) {
    drawBitmap(logo, xLocation, yLocation);  
}

void OledDriver::showMessageAtLine(const char* message, const int16_t line) {
    _display.setCursor(0, line * 8);
    _display.print(message);
    _needsDisplay = true;
}

void OledDriver::switchEventLogo(const unsigned char* logo, const long switchOn) {
    switchLogo(logo, EVENT_X, EVENT_Y, switchOn);
}

void OledDriver::switchFlowLogo(const unsigned char* logo, const long switchOn) {
    switchLogo(logo, FLOW_X, FLOW_Y, switchOn);
}

void OledDriver::switchLogo(const unsigned char* logo, const int16_t xLocation, const int16_t yLocation, const long switchOn) {
     if (switchOn) {
        setLogo(logo, xLocation, yLocation);
     } else {
        clearLogo(xLocation, yLocation);
     }
}

void OledDriver::update(Topic topic, const char* payload) {
    if(topic == Topic::Meter) {
        char buffer[20];
        safeSprintf(buffer, "%s m3 ", payload); // %012.6lf
        showMessageAtLine(buffer, 1);
    }
}

void OledDriver::update(Topic topic, long payload) {
    switch(topic) {
        char buffer[20];
    case Topic::Connection:
        switch (static_cast<ConnectionState>(payload)) {
            case ConnectionState::CheckFirmware:
                setConnectionLogo(FIRMWARE_LOGO);
                return;
            case ConnectionState::Disconnected:
                clearConnectionLogo();
                return;
            case ConnectionState::MqttReady:
                setConnectionLogo(MQTT_LOGO);
                return;
            case ConnectionState::RequestTime:
                setConnectionLogo(TIME_LOGO);
                return;
            case ConnectionState::WifiReady:
                setConnectionLogo(WIFI_LOGO);
                return;
            default:
                return;
        }
    case Topic::Flow:
        switchFlowLogo(FLOW_LOGO, payload);
        return;
    case Topic::Alert:
        switchEventLogo(ALERT_LOGO, payload);
        return;
    case Topic::Blocked:
        switchEventLogo(BLOCKED_LOGO, payload);
        return;
    case Topic::SensorWasReset:
        switchEventLogo(RESET_LOGO, payload);
        return;
    case Topic::NoSensorFound:
        switchEventLogo(NO_SENSOR_LOGO, payload);
        return;
    case Topic::TimeOverrun:
        switchEventLogo(TIME_LOGO, payload);
        return;        

    case Topic::Peaks: 
        safeSprintf(buffer, "Peaks: %ld", payload);
        showMessageAtLine(buffer, 0);
  }
}