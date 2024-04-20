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


#include <Wire.h>
#include "OledDriver.h"
#include "ConnectionState.h"
#include <SafeCString.h>

// ReSharper disable CppClangTidyReadabilityRedundantDeclaration - needed for compilation on device (C++ 11)

namespace WaterMeter {
    constexpr unsigned char OledDriver::AlertLogo[];
    constexpr unsigned char OledDriver::DownloadLogo[];
    constexpr unsigned char OledDriver::FirmwareLogo[];
    constexpr unsigned char OledDriver::BlockedLogo[];
    constexpr unsigned char OledDriver::MqttLogo[];
    constexpr unsigned char OledDriver::NoSensorLogo[];
    constexpr unsigned char OledDriver::NoFitLogo[];
    constexpr unsigned char OledDriver::OutlierLogo[];
    constexpr unsigned char OledDriver::ResetLogo[];
    constexpr unsigned char OledDriver::SendLogo[];
    constexpr unsigned char OledDriver::TimeLogo[];
    constexpr unsigned char OledDriver::WifiLogo[];

    OledDriver::OledDriver(EventServer* eventServer, TwoWire* wire) :
        EventClient(eventServer),
        _wire(wire),
        _display(ScreenWidth, ScreenHeight, wire) {
        eventServer->subscribe(this, Topic::Begin);
    }

    // for testing only
    Adafruit_SSD1306* OledDriver::getDriver() { return &_display; }

    bool OledDriver::begin() {
        constexpr int Oled128X32 = 0x3c;
        // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
        // Do an explicit check whether the oled is there (the driver doesn't do that)
        _wire->beginTransmission(Oled128X32);
        // ReSharper disable once CppRedundantParentheses -- done to show intent
        if ((_wire->endTransmission() != 0) || !_display.begin(SSD1306_SWITCHCAPVCC, Oled128X32, false, false)) {
            _eventServer->publish(Topic::NoDisplayFound, true);
            return false;
        }
        _display.clearDisplay();
        _display.setTextSize(1);
        _display.setTextColor(WHITE, BLACK); // white text on black background
        _display.cp437(true); // Use 'Code Page 437' font
        showMessageAtLine("Waiting", 3);
        _display.display();
        _needsDisplay = false;
        _eventServer->subscribe(this, Topic::Alert);
        _eventServer->subscribe(this, Topic::Blocked);
        _eventServer->subscribe(this, Topic::Connection);
        _eventServer->subscribe(this, Topic::NoFit);
        _eventServer->subscribe(this, Topic::SensorState);
        _eventServer->subscribe(this, Topic::SensorWasReset);
        _eventServer->subscribe(this, Topic::TimeOverrun);
        _eventServer->subscribe(this, Topic::UpdateProgress);
        _eventServer->subscribe(this, Topic::Volume);
        _eventServer->subscribe(this, Topic::Pulses);
        delay(MinDelayForDisplay);
        return true;
    }

    void OledDriver::clearConnectionLogo() {
        clearLogo(ConnectionX, ConnectionY);
    }

    void OledDriver::clearLogo(const int16_t xLocation, const int16_t yLocation) {
        _display.fillRect(xLocation, yLocation, LogoWidth, LogoHeight, BLACK);
        _needsDisplay = false;
    }

    unsigned int OledDriver::display() {
        if (_needsDisplay) {
            _display.display();
            _needsDisplay = false;
            delay(MinDelayForDisplay);
            return MinDelayForDisplay;
        }
        return 0;
    }

    void OledDriver::drawBitmap(const unsigned char* logo, const int16_t xLocation, const int16_t yLocation) {
        _display.drawBitmap(xLocation, yLocation, logo, LogoWidth, LogoHeight, WHITE, BLACK);
        _needsDisplay = true;
    }

    void OledDriver::setConnectionLogo(const unsigned char* logo) {
        setLogo(logo, ConnectionX, ConnectionY);
    }

    void OledDriver::setLogo(const unsigned char* logo, const int16_t xLocation, const int16_t yLocation) {
        drawBitmap(logo, xLocation, yLocation);
    }

    void OledDriver::showMessageAtLine(const char* message, const int16_t line) {
        _display.setCursor(0, static_cast<int16_t>(LineHeight * line));
        _display.print(message);
        _needsDisplay = true;
    }

    void OledDriver::switchEventLogo(const unsigned char* logo, const long switchOn) {
        switchLogo(logo, EventX, EventY, switchOn);
    }

    void OledDriver::switchFlowLogo(const unsigned char* logo, const long switchOn) {
        switchLogo(logo, FlowX, FlowY, switchOn);
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
            setConnectionLogo(FirmwareLogo);
            return;
        case ConnectionState::Disconnected:
            clearConnectionLogo();
            return;
        case ConnectionState::MqttReady:
            setConnectionLogo(MqttLogo);
            return;
        case ConnectionState::RequestTime:
            setConnectionLogo(TimeLogo);
            return;
        case ConnectionState::WifiReady:
            setConnectionLogo(WifiLogo);
            // ReSharper disable once CppRedundantControlFlowJump - would introduce a fall-though warning
            return;
        default:;
            // do nothing, intermediate state
        }
    }

    void OledDriver::update(const Topic topic, const char* payload) {
        if (topic == Topic::Volume) {
            char buffer[20];
            SafeCString::sprintf(buffer, "%s m3 ", payload);
            showMessageAtLine(buffer, 1);
        }
    }

    void OledDriver::update(const Topic topic, long payload) {
        char buffer[20];
        switch (topic) {
        case Topic::Begin:
            // this one must be done after logging has started, controlled via payload.
            if (payload) {
                begin();
            }
            break;
        case Topic::Connection:
            updateConnectionState(static_cast<ConnectionState>(payload));
            return;
        case Topic::Alert:
            switchEventLogo(AlertLogo, payload);
            return;
        case Topic::Blocked:
            switchEventLogo(BlockedLogo, payload);
            return;
        case Topic::SensorWasReset:
            if (payload == 2) showMessageAtLine("Hard reset       ", 3);
            else showMessageAtLine("Soft reset       ", 3);
            switchEventLogo(ResetLogo, payload);
            return;
        case Topic::NoFit:
            SafeCString::sprintf(buffer, "No fit: %4ld deg ", payload);
            showMessageAtLine(buffer, 3);
            switchFlowLogo(NoFitLogo, payload);
            return;
        case Topic::SensorState:
            switch (payload) {
            case 0:
                showMessageAtLine("No sensor        ", 3);
                break;
            case 1:
                showMessageAtLine("Sensor OK        ", 3);
                break;
            case 2:
                showMessageAtLine("Power error      ", 3);
                break;
            case 3:
                showMessageAtLine("Begin error      ", 3);
                break;
            default:; // should not occur
            }
            switchFlowLogo(NoSensorLogo, payload != 1);
            return;
        case Topic::TimeOverrun:
            if (payload == 0) return;
            SafeCString::sprintf(buffer, "Overrun: %8ld", payload);
            showMessageAtLine(buffer, 3);
            switchEventLogo(TimeLogo, payload);
            return;
        case Topic::UpdateProgress:
            SafeCString::sprintf(buffer, "FW update: %d%% ", payload);
            showMessageAtLine(buffer, 2);
            return;
        case Topic::Pulses:
            SafeCString::sprintf(buffer, "Pulses: %7ld", payload);
            showMessageAtLine(buffer, 0);
            // ReSharper disable once CppRedundantControlFlowJump - would introduce a fall-though warning
            return;
        default:;
            // do nothing
        }
    }
}