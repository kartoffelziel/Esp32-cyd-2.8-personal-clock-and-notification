#pragma once
#include <stdint.h>

struct Calibration {
    uint8_t valid;
    uint8_t swap;               // screen X follows the raw Y channel
    int16_t a0, a1;             // raw values at screen x = 20 and x = 299
    int16_t b0, b1;             // raw values at screen y = 20 and y = 219
};

struct Settings {
    uint32_t magic;
    uint16_t version;
    // General
    uint8_t backlight;          // 10..255
    uint8_t bgBrightness;       // 0..100 (%)
    uint8_t autoSky;            // 1 = colour follows sunrise / day / sunset / night
    uint16_t bgColor;           // solid background when autoSky is off
    uint8_t powerMode;          // 0 = screen off, 1 = sleep (radios off, saves power)
    uint8_t autoHideSec;        // 0 = never hide the buttons
    uint8_t flipped;            // rotate 180 degrees
    // Clock
    uint8_t clockFont;          // 0..kDigitFontCount (last = LED)
    uint8_t clockSize;          // digit height in pixels
    int16_t clockX, clockY;     // offset from the centred position
    uint8_t hour24;
    uint8_t leadingZero;
    uint8_t colonBlink;
    uint8_t clockEffect;        // 0 none, 1 fade on change, 2 rainbow, 3 breathing
    uint16_t clockColor;
    uint8_t showDate;
    uint8_t dateSize;           // 0..3
    uint8_t dateFont;           // family index in kDateFontNames
    // Connect
    char ssid[33];
    char pass[65];
    int8_t tzHalfHours;         // GMT offset in 30 minute units (+7 = 14)
    uint8_t wifiAuto;
    uint8_t bleTime;            // phone time sync over BLE (reserved)
    // Notification
    uint8_t ntfPopup;
    uint8_t ntfTicker;
    uint8_t ntfSize;            // 0..2
    uint8_t ntfEffect;          // 0 none, 1 slide, 2 border flash, 3 screen flash
    uint8_t ntfSeconds;
    uint16_t ntfBg;
    uint16_t ntfFg;
    Calibration cal;
};

void settingsDefaults(Settings &s);
void settingsLoad(Settings &s);
void settingsSave(const Settings &s);
