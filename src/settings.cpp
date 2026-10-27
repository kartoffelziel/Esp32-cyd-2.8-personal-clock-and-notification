#include "settings.h"

#include "gfx.h"
#include "fonts.h"
#include "hal.h"
#include <string.h>

static const uint32_t kMagic = 0x43594331;  // "CYC1"
static const uint16_t kVersion = 2;

void settingsDefaults(Settings &s) {
    memset(&s, 0, sizeof(s));
    s.magic = kMagic;
    s.version = kVersion;
    s.backlight = 200;
    s.bgBrightness = 70;
    s.autoSky = 1;
    s.bgColor = rgb(0, 0, 0);
    s.powerMode = 0;
    s.autoHideSec = 15;
    s.flipped = 0;
    s.clockFont = 2;            // "Hẹp"
    s.clockSize = 100;
    s.clockX = 0;
    s.clockY = 0;
    s.hour24 = 1;
    s.leadingZero = 1;
    s.colonBlink = 1;
    s.clockEffect = 0;
    s.clockColor = rgb(255, 255, 255);
    s.showDate = 1;
    s.dateSize = 1;
    s.dateFont = 0;
    s.tzHalfHours = 14;         // GMT+7
    s.wifiAuto = 1;
    s.bleTime = 1;
    s.ntfPopup = 1;
    s.ntfTicker = 1;
    s.ntfSize = 1;
    s.ntfEffect = 1;
    s.ntfSeconds = 8;
    s.ntfBg = rgb(24, 32, 64);
    s.ntfFg = rgb(255, 255, 255);
    s.cal.valid = 0;
}

void settingsLoad(Settings &s) {
    Settings loaded;
    const size_t n = hal_store_read("cfg", &loaded, sizeof(loaded));
    if (n == sizeof(loaded) && loaded.magic == kMagic && loaded.version == kVersion) {
        s = loaded;
        // hard limits so a corrupted blob can never break the UI
        if (s.backlight < 10) s.backlight = 10;
        if (s.bgBrightness > 100) s.bgBrightness = 100;
        if (s.clockSize < 24) s.clockSize = 24;
        if (s.dateSize > 3) s.dateSize = 3;
        if (s.dateFont >= kDateFontCount) s.dateFont = 0;
        if (s.ntfSize > 2) s.ntfSize = 2;
        s.ssid[sizeof(s.ssid) - 1] = 0;
        s.pass[sizeof(s.pass) - 1] = 0;
    } else {
        settingsDefaults(s);
    }
}

void settingsSave(const Settings &s) { hal_store_write("cfg", &s, sizeof(s)); }
