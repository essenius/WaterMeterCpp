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

// ReSharper disable CppClangTidyReadabilityRedundantDeclaration - needed for compilation on device (C++ 11)

constexpr unsigned char OledDriver::ALERT_LOGO[];
constexpr unsigned char OledDriver::DOWNLOAD_LOGO[];
constexpr unsigned char OledDriver::FIRMWARE_LOGO[];
constexpr unsigned char OledDriver::BLOCKED_LOGO[];
constexpr unsigned char OledDriver::MQTT_LOGO[];
constexpr unsigned char OledDriver::NO_SENSOR_LOGO[];
constexpr unsigned char OledDriver::OUTLIER_LOGO[];
constexpr unsigned char OledDriver::RESET_LOGO[];
constexpr unsigned char OledDriver::SEND_LOGO[];
constexpr unsigned char OledDriver::TIME_LOGO[];
constexpr unsigned char OledDriver::WIFI_LOGO[];

OledDriver::OledDriver(EventServer* eventServer, TwoWire* wire) :
    EventClient(eventServer),
    _wire(wire),
    _display(SCREEN_WIDTH, SCREEN_HEIGHT, wire) {}

// for testing only
Adafruit_SSD1306* OledDriver::getDriver() { return &_display; }

bool OledDriver::begin() {
    constexpr int OLED_128_32 = 0x3c;
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    // Do an explicit check whether the oled is there (the driver doesn't do that)
    _wire->beginTransmission(OLED_128_32);
    // ReSharper disable once CppRedundantParentheses -- done to show intent
    if ((_wire->endTransmission() != 0) || !_display.begin(SSD1306_SWITCHCAPVCC, OLED_128_32, false, false)) {
        _eventServer->publish(Topic::NoDisplayFound, LONG_TRUE);
        return false;
    }
    _display.clearDisplay();
    _display.setTextSize(1);
    _display.setTextColor(WHITE, BLACK); // white text on black background
    _display.cp437(true); // Use full 256 char 'Code Page 437' font
    showMessageAtLine("Waiting", 3);
    _display.display();
    _needsDisplay = false;
    _eventServer->subscribe(this, Topic::Alert);
    _eventServer->subscribe(this, Topic::Blocked);
    _eventServer->subscribe(this, Topic::Connection);
    _eventServer->subscribe(this, Topic::NoSensorFound);
    _eventServer->subscribe(this, Topic::SensorWasReset);
    _eventServer->subscribe(this, Topic::TimeOverrun);
    _eventServer->subscribe(this, Topic::UpdateProgress);
    _eventServer->subscribe(this, Topic::Volume);
    _eventServer->subscribe(this, Topic::Pulses);
    delay(MIN_DELAY_FOR_DISPLAY);
    return true;
}

void OledDriver::clearConnectionLogo() {
    clearLogo(118, 0);
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
    _display.setCursor(0, static_cast<int16_t>(LINE_HEIGHT * line));
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
    }
    else {
        clearLogo(xLocation, yLocation);
    }
}

void OledDriver::updateConnectionState(const ConnectionState payload) {

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
        // ReSharper disable once CppRedundantControlFlowJump - would introduce a fallthough warning
        return;
    default: ;
    // do nothing, intermediate state
    }
}

void OledDriver::update(const Topic topic, const char* payload) {
    if (topic == Topic::Volume) {
        char buffer[20];
        safeSprintf(buffer, "%s m3 ", payload);
        showMessageAtLine(buffer, 1);
    }
}

void OledDriver::update(const Topic topic, long payload) {
    char buffer[20];
    switch (topic) {
    case Topic::Begin:
        // this one must be done after logging has started, controlled via payload.
        if (payload == LONG_TRUE) {
            begin();
        }
        break;
    case Topic::Connection:
        updateConnectionState(static_cast<ConnectionState>(payload));
        return;
    case Topic::Alert:
        switchEventLogo(ALERT_LOGO, payload);
        return;
    case Topic::Blocked:
        switchEventLogo(BLOCKED_LOGO, payload);
        return;
    case Topic::SensorWasReset:
        if (payload == 2) showMessageAtLine("Hard reset       ", 3);
        else showMessageAtLine("Soft reset       ", 3);
        switchEventLogo(RESET_LOGO, payload);
        return;
    case Topic::NoSensorFound:
        showMessageAtLine("No sensor        ", 3);
        switchFlowLogo(NO_SENSOR_LOGO, payload);
        return;
    case Topic::TimeOverrun:
        safeSprintf(buffer, "Overrun: %8ld", payload);
        showMessageAtLine(buffer, 3);
        switchEventLogo(TIME_LOGO, payload);
        return;
    case Topic::UpdateProgress:
        safeSprintf(buffer, "FW update: %d%% ", payload);
        showMessageAtLine(buffer, 2);
        return;
    case Topic::Pulses:
        safeSprintf(buffer, "Pulses: %7ld", payload);
        showMessageAtLine(buffer, 0);
        // ReSharper disable once CppRedundantControlFlowJump - would introduce a fallthough warning
        return;
    default: ;
    // do nothing
    }
}
