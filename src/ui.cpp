#include "ui.h"

#include <algorithm>
#include <stdio.h>
#include <string.h>

namespace {
constexpr int W = 320;
constexpr int H = 240;

const uint16_t C_BG = rgb(10, 14, 24);
const uint16_t C_ROW = rgb(22, 28, 44);
const uint16_t C_ROW2 = rgb(18, 23, 37);
const uint16_t C_ACC = rgb(0, 150, 255);
const uint16_t C_TXT = rgb(240, 244, 250);
const uint16_t C_MUT = rgb(140, 150, 170);
const uint16_t C_OK = rgb(60, 205, 115);
const uint16_t C_BAD = rgb(235, 80, 80);
const uint16_t C_TRACK = rgb(52, 62, 88);
const uint16_t C_BTN = rgb(34, 44, 70);

const uint16_t kTextPalette[8] = {rgb(255, 255, 255), rgb(255, 214, 10), rgb(255, 140, 30), rgb(240, 70, 70),
                                  rgb(60, 220, 120), rgb(60, 220, 230), rgb(80, 140, 255), rgb(190, 110, 255)};
const uint16_t kBgPalette[8] = {rgb(0, 0, 0), rgb(26, 26, 30), rgb(8, 16, 56), rgb(0, 44, 52),
                                rgb(8, 44, 22), rgb(56, 12, 18), rgb(40, 16, 62), rgb(48, 30, 12)};
const uint16_t kCardPalette[8] = {rgb(24, 32, 64), rgb(20, 70, 44), rgb(96, 32, 32), rgb(64, 44, 100),
                                  rgb(34, 34, 38), rgb(10, 84, 116), rgb(120, 76, 10), rgb(214, 216, 224)};

const TextFace kNoticeFaces[3] = {TF_R14, TF_R18, TF_R24};

const char *const kWeekdayLong[7] = {"Chủ nhật", "Thứ hai", "Thứ ba", "Thứ tư", "Thứ năm", "Thứ sáu", "Thứ bảy"};
const char *const kWeekdayShort[7] = {"CN", "T2", "T3", "T4", "T5", "T6", "T7"};

const char *const kTabNames[4] = {"Chung", "Kết nối", "Đồng hồ", "Thông báo"};

const char *const kPowerNames[2] = {"Tắt màn hình", "Ngủ (tiết kiệm)"};
const char *const kEffectNames[4] = {"Không", "Mờ dần khi đổi", "Cầu vồng", "Thở"};
const char *const kDateSizeNames[4] = {"Nhỏ", "Vừa", "Lớn", "Rất lớn"};
const char *const kNtfSizeNames[3] = {"Nhỏ", "Vừa", "Lớn"};
const char *const kNtfEffectNames[4] = {"Không", "Trượt vào", "Nháy viền", "Nháy màn hình"};

// Tab boundaries from the measured label widths; the close button owns x >= 280.
const int *tabX() {
    static int x[5];
    static bool init = false;
    if (!init) {
        int w[4], sum = 0;
        for (int i = 0; i < 4; ++i) { w[i] = textWidth(TF_R14, kTabNames[i]) + 8; sum += w[i]; }
        const int extra = (280 - sum) / 4;
        int cur = 0;
        for (int i = 0; i < 4; ++i) { x[i] = cur; cur += w[i] + (extra > 0 ? extra : 0); }
        x[4] = 280;
        init = true;
    }
    return x;
}

inline bool inRect(int x, int y, int rx, int ry, int rw, int rh) {
    return x >= rx && x < rx + rw && y >= ry && y < ry + rh;
}

void fillBgRows(const Bg &bg, int x, int y, int w, int h) {
    for (int yy = y; yy < y + h; ++yy)
        if (yy >= 0 && yy < H) gfx().fillRect(x, yy, w, 1, bg.at(yy));
}

void drawGear(int cx, int cy, uint16_t c) {
    Gfx &g = gfx();
    g.fillCircle(cx, cy, 9, c);
    for (int i = 0; i < 8; ++i) {
        const int dx[8] = {0, 7, 10, 7, 0, -7, -10, -7};
        const int dy[8] = {-10, -7, 0, 7, 10, 7, 0, -7};
        g.fillCircle(cx + dx[i], cy + dy[i], 2, c);
    }
}

void drawPowerIcon(int cx, int cy, uint16_t c, uint16_t bgc) {
    Gfx &g = gfx();
    g.drawCircle(cx, cy, 10, c);
    g.drawCircle(cx, cy, 9, c);
    g.fillRect(cx - 4, cy - 12, 9, 9, bgc);
    g.fillRect(cx - 1, cy - 11, 3, 10, c);
}

void drawArrow(int cx, int cy, int dir, int r, uint16_t c) {   // 0 up 1 right 2 down 3 left
    Gfx &g = gfx();
    switch (dir) {
        case 0: g.fillTriangle(cx, cy - r, cx - r, cy + r / 2, cx + r, cy + r / 2, c); break;
        case 1: g.fillTriangle(cx + r, cy, cx - r / 2, cy - r, cx - r / 2, cy + r, c); break;
        case 2: g.fillTriangle(cx, cy + r, cx - r, cy - r / 2, cx + r, cy - r / 2, c); break;
        default: g.fillTriangle(cx - r, cy, cx + r / 2, cy - r, cx + r / 2, cy + r, c); break;
    }
}

void button(int x, int y, int w, int h, const char *label, uint16_t fill, uint16_t txt, TextFace face = TF_B14) {
    gfx().fillRoundRect(x, y, w, h, 8, fill);
    drawTextCentered(face, x + w / 2, y + (h - textHeight(face)) / 2, label, txt, Bg::solid(fill));
}

}  // namespace

struct App::Geom {
    int H = 100, cell = 50, colon = 36, hn = 2, x0 = 0, y0 = 0, totalW = 0, dateY = 0;
    int cellX[4] = {0, 0, 0, 0};
    int colonX = 0;
    int ampm = -1;     // -1 24h, 0 AM, 1 PM
};

bool App::timeOk() const { return hal_time_valid(); }

LocalTime App::now(uint32_t) const {
    if (!hal_time_valid()) return localTimeFromUnix(0, 0, 0);
    const int64_t ms = hal_unix_ms();
    const int64_t secs = ms >= 0 ? ms / 1000 : -((-ms + 999) / 1000);
    return localTimeFromUnix(secs, s.tzHalfHours * 30, (int)(ms - secs * 1000));
}

App::Geom App::geom() const {
    Geom g;
    const bool ok = timeOk();
    const LocalTime t = now(0);
    int hv = t.hour;
    if (!s.hour24) { hv = t.hour % 12; if (hv == 0) hv = 12; g.ampm = t.hour >= 12 ? 1 : 0; }
    g.hn = (!ok || s.leadingZero || hv >= 10) ? 2 : 1;
    const int cap = maxClockHeight(s.clockFont, W, 4);
    g.H = std::min<int>(s.clockSize, cap);
    g.cell = digitCellWidth(s.clockFont, g.H);
    g.colon = colonCellWidth(s.clockFont, g.H);
    g.totalW = (g.hn + 2) * g.cell + g.colon;
    const int dateH = s.showDate ? 8 + textHeight(dateFace(s.dateFont, s.dateSize)) : 0;
    g.x0 = W / 2 - g.totalW / 2 + s.clockX;
    g.y0 = (H - (g.H + dateH)) / 2 + s.clockY;
    g.dateY = g.y0 + g.H + 8;
    for (int i = 0; i < 4; ++i) {
        if (i < 2) g.cellX[i] = g.hn == 2 ? g.x0 + i * g.cell : (i == 1 ? g.x0 : -1);
        else g.cellX[i] = g.x0 + g.hn * g.cell + g.colon + (i - 2) * g.cell;
    }
    g.colonX = g.x0 + g.hn * g.cell;
    return g;
}

// ------------------------------------------------------------------------------------------
// Background / colours
// ------------------------------------------------------------------------------------------
void App::computeBg() {
    uint16_t top, bottom;
    if (s.autoSky) {
        int minute = 21 * 60;
        if (timeOk()) { const LocalTime t = now(0); minute = t.hour * 60 + t.minute; }
        skyForMinute(minute, top, bottom);
    } else {
        top = bottom = s.bgColor;
    }
    skyTop_ = scaleColor(top, s.bgBrightness);
    skyBot_ = scaleColor(bottom, s.bgBrightness);
    bg_ = Bg{skyTop_, skyBot_, 0, H - 1};
}

uint16_t App::digitColor(uint32_t nowMs) const {
    if (!timeOk()) return rgb(120, 126, 140);
    if (s.clockEffect == 2) return hsv565((int)((nowMs / 120) * 6 % 360), 255, 255);
    return s.clockColor;
}

int App::digitStrength(int cell, uint32_t nowMs) const {
    if (s.clockEffect == 1) {
        const uint32_t dt = nowMs - fadeStart_[cell];
        return dt >= 450 ? 255 : (int)(dt * 255 / 450);
    }
    if (s.clockEffect == 3) {
        const int ph = (int)(nowMs % 4000);
        const int tri = ph < 2000 ? ph : 4000 - ph;
        return (140 + tri * 115 / 2000) / 8 * 8;
    }
    return 255;
}

void App::drawBackground() { fillBgRows(bg_, 0, 0, W, H); }

void App::drawButtons() {
    if (!uiVisible_) return;
    const uint16_t pad = scaleColor(rgb(255, 255, 255), 16);
    gfx().fillCircle(24, 24, 19, mix565(bg_.at(24), rgb(255, 255, 255), 40));
    gfx().fillCircle(W - 24, 24, 19, mix565(bg_.at(24), rgb(255, 255, 255), 40));
    (void)pad;
    drawGear(24, 24, rgb(255, 255, 255));
    drawPowerIcon(W - 24, 24, rgb(255, 255, 255), mix565(bg_.at(24), rgb(255, 255, 255), 40));
}

void App::drawDateLine() {
    const Geom g = geom();
    const bool ok = timeOk();
    const TextFace face = dateFace(s.dateFont, s.dateSize);
    fillBgRows(bg_, 0, g.dateY, W, textHeight(face) + 2);
    if (!s.showDate && ok) return;
    char buf[64];
    if (!ok) {
        snprintf(buf, sizeof(buf), "Chưa có giờ - nối WiFi hoặc điện thoại");
        drawTextCentered(TF_R14, W / 2, g.dateY, buf, rgb(190, 196, 210), bg_);
        return;
    }
    const LocalTime t = now(0);
    snprintf(buf, sizeof(buf), "%s, %02d/%02d/%04d", kWeekdayLong[t.weekday], t.day, t.month, t.year);
    if (textWidth(face, buf) > W - 8) snprintf(buf, sizeof(buf), "%s, %02d/%02d/%04d", kWeekdayShort[t.weekday], t.day, t.month, t.year);
    if (textWidth(face, buf) > W - 8) snprintf(buf, sizeof(buf), "%02d/%02d/%04d", t.day, t.month, t.year);
    drawTextCentered(face, W / 2 + s.clockX, g.dateY, buf, s.clockColor, bg_);
}

void App::drawClockAll() {
    const Geom g = geom();
    const bool ok = timeOk();
    const uint32_t ms = hal_millis();
    const LocalTime t = now(ms);
    int hv = t.hour;
    if (!s.hour24) { hv = t.hour % 12; if (hv == 0) hv = 12; }
    const int vals[4] = {ok ? hv / 10 : -2, ok ? hv % 10 : -2, ok ? t.minute / 10 : -2, ok ? t.minute % 10 : -2};
    const uint16_t color = digitColor(ms);
    for (int i = 0; i < 4; ++i) {
        if (g.cellX[i] < 0) { shown_[i] = -1; continue; }
        const int strength = digitStrength(i, ms);
        drawDigit(s.clockFont, g.H, vals[i], g.cellX[i], g.y0, color, strength, bg_);
        shown_[i] = vals[i];
        shownStrength_[i] = strength;
    }
    const bool colonOn = !ok || !s.colonBlink || t.millisecond < 500;
    drawColon(s.clockFont, g.H, colonOn, g.colonX, g.y0, color, 255, bg_);
    shownColon_ = colonOn ? 1 : 0;
    shownColor_ = color;
    shownAmPm_ = -1;
    if (ok && !s.hour24) {
        const int ampm = t.hour >= 12 ? 1 : 0;
        drawText(TF_R18, g.x0 + g.totalW + 4, g.y0 + g.H - textHeight(TF_R18), ampm ? "CH" : "SA", color, bg_);
        shownAmPm_ = ampm;
    }
}

void App::drawMainFull() {
    drawBackground();
    drawClockAll();
    drawDateLine();
    const LocalTime t = now(0);
    shownDateKey_ = timeOk() ? (t.year * 10000 + t.month * 100 + t.day) : 0;
    drawButtons();
}

void App::updateMain() {
    const uint32_t ms = hal_millis();
    const bool ok = timeOk();
    const Bg oldBg = bg_;
    computeBg();
    if (bg_.top != oldBg.top || bg_.bottom != oldBg.bottom) dirty_ = true;
    const Geom g = geom();
    const int layout = (g.hn * 31 + g.H) * 31 + s.clockFont * 7 + (g.x0 + 500) * 13 + (g.y0 + 500) * 17 +
                       s.showDate * 3 + s.dateSize * 5 + s.dateFont * 41 + (ok ? 1 : 0) + (uiVisible_ ? 2 : 0) * 977 +
                       (s.hour24 ? 11 : 0) + s.clockColor * 3;
    if (layout != shownLayout_) { dirty_ = true; shownLayout_ = layout; }
    if (dirty_) {
        dirty_ = false;
        drawMainFull();
        return;
    }
    const LocalTime t = now(ms);
    int hv = t.hour;
    if (!s.hour24) { hv = t.hour % 12; if (hv == 0) hv = 12; }
    const int vals[4] = {ok ? hv / 10 : -2, ok ? hv % 10 : -2, ok ? t.minute / 10 : -2, ok ? t.minute % 10 : -2};
    const uint16_t color = digitColor(ms);
    for (int i = 0; i < 4; ++i) {
        if (g.cellX[i] < 0) continue;
        if (vals[i] != shown_[i] && shown_[i] != -9 && s.clockEffect == 1) fadeStart_[i] = ms;
        const int strength = digitStrength(i, ms);
        if (vals[i] != shown_[i] || strength != shownStrength_[i] || color != shownColor_) {
            drawDigit(s.clockFont, g.H, vals[i], g.cellX[i], g.y0, color, strength, bg_);
            shown_[i] = vals[i];
            shownStrength_[i] = strength;
        }
    }
    const bool colonOn = !ok || !s.colonBlink || t.millisecond < 500;
    if ((colonOn ? 1 : 0) != shownColon_ || color != shownColor_) {
        drawColon(s.clockFont, g.H, colonOn, g.colonX, g.y0, color, 255, bg_);
        shownColon_ = colonOn ? 1 : 0;
    }
    shownColor_ = color;
    const int dateKey = ok ? (t.year * 10000 + t.month * 100 + t.day) : 0;
    if (dateKey != shownDateKey_) { drawDateLine(); shownDateKey_ = dateKey; }
    if (ok && !s.hour24) {
        const int ampm = t.hour >= 12 ? 1 : 0;
        if (ampm != shownAmPm_) {
            drawText(TF_R18, g.x0 + g.totalW + 4, g.y0 + g.H - textHeight(TF_R18), ampm ? "CH" : "SA", color, bg_);
            shownAmPm_ = ampm;
        }
    }
}

// ------------------------------------------------------------------------------------------
// Lifecycle, touch plumbing, power
// ------------------------------------------------------------------------------------------
namespace {
void mapTouch(const Calibration &c, int rx, int ry, int &x, int &y) {
    const int ax = c.swap ? ry : rx;
    const int ay = c.swap ? rx : ry;
    int da = c.a1 - c.a0;
    int db = c.b1 - c.b0;
    if (da == 0) da = 1;
    if (db == 0) db = 1;
    x = 20 + (ax - c.a0) * 279 / da;
    y = 20 + (ay - c.b0) * 199 / db;
    x = x < 0 ? 0 : (x > W - 1 ? W - 1 : x);
    y = y < 0 ? 0 : (y > H - 1 ? H - 1 : y);
}
}  // namespace

void App::begin() {
    settingsLoad(s);
    hal_set_flipped(s.flipped);
    hal_backlight(s.backlight);
    if (s.wifiAuto && s.ssid[0]) hal_wifi_connect(s.ssid, s.pass);
    lastInteract_ = hal_millis();
    if (!s.cal.valid) startCalibration();
    else backToMain();
}

void App::backToMain() {
    screen_ = SCR_MAIN;
    dirty_ = true;
    uiVisible_ = true;
    lastInteract_ = hal_millis();
    shownLayout_ = -1;
}

void App::wake() {
    screenOff_ = false;
    hal_backlight(s.backlight);
    ignoreUntilUp_ = true;
    dirty_ = true;
    uiVisible_ = true;
    lastInteract_ = hal_millis();
    shownLayout_ = -1;
    if (screen_ == SCR_SETTINGS) drawSettingsFull();
    else if (screen_ == SCR_CALIB) drawCalib();
}

void App::doPower() {
    commit();
    screenOff_ = true;
    hal_backlight(0);
    if (s.powerMode == 1) {
        hal_sleep_until_touch();
        if (s.wifiAuto && s.ssid[0]) hal_wifi_connect(s.ssid, s.pass);
        wake();
    }
}

void App::tick() {
    static int miss = 0;
    const uint32_t ms = hal_millis();
    int rx = 0, ry = 0;
    const bool raw = hal_touch_read(rx, ry);
    if (raw) { stable_ = std::min(stable_ + 1, 4); miss = 0; }
    else { stable_ = 0; miss = std::min(miss + 1, 4); }
    const bool pressed = stable_ >= 2;
    const bool released = miss >= 3;

    if (screen_ == SCR_CALIB && !calTest_) {
        calibRaw(pressed, rx, ry);
    } else {
        const Calibration &c = (screen_ == SCR_CALIB) ? calTmp_ : s.cal;
        if (pressed) {
            int x, y;
            mapTouch(c, rx, ry, x, y);
            if (!pressed_) {
                pressed_ = true;
                downX_ = lastX_ = x;
                downY_ = lastY_ = y;
                downMs_ = ms;
                handleTouch({TouchEvt::DOWN, x, y});
            } else if (abs(x - lastX_) >= 2 || abs(y - lastY_) >= 2) {
                lastX_ = x;
                lastY_ = y;
                handleTouch({TouchEvt::MOVE, x, y});
            }
        } else if (pressed_ && released) {
            pressed_ = false;
            handleTouch({TouchEvt::UP, lastX_, lastY_});
        }
    }

    if (screenOff_) return;

    // network / time bookkeeping once a second
    if (ms - lastPoll_ >= 1000) {
        lastPoll_ = ms;
        const int st = hal_wifi_status();
        if (st == WIFI_CONNECTED && !ntpRequested_) { hal_ntp_request(); ntpRequested_ = true; }
        if (st != WIFI_CONNECTED) ntpRequested_ = false;
    }

    switch (screen_) {
        case SCR_MAIN: {
            if (uiVisible_ && s.autoHideSec && ms - lastInteract_ > (uint32_t)s.autoHideSec * 1000U && !notice_.active) {
                uiVisible_ = false;
                dirty_ = true;
            }
            static uint32_t lastMain = 0;
            if (ms - lastMain >= 40) {
                lastMain = ms;
                updateNotice();
                const bool popup = notice_.active && s.ntfPopup;
                if (!popup) updateMain();
                if (notice_.active) drawNotice(ms);
            }
            break;
        }
        case SCR_SETTINGS:
            pollConnect();
            break;
        case SCR_ADJUST:
            if (adjustHold_ >= 0 && ms >= adjustNext_) {
                adjustStep(adjustHold_);
                adjustNext_ = ms + 90;
            }
            break;
        default:
            break;
    }
}

void App::handleTouch(const TouchEvt &e) {
    if (e.kind == TouchEvt::DOWN) lastInteract_ = hal_millis();
    if (screenOff_) {
        if (e.kind == TouchEvt::DOWN) wake();
        return;
    }
    if (ignoreUntilUp_) {
        if (e.kind == TouchEvt::UP) ignoreUntilUp_ = false;
        return;
    }
    switch (screen_) {
        case SCR_MAIN: mainTouch(e); break;
        case SCR_SETTINGS: settingsTouch(e); break;
        case SCR_ADJUST: adjustTouch(e); break;
        case SCR_KEYBOARD: keyboardTouch(e); break;
        case SCR_CALIB: calibTouch(e); break;
    }
}

void App::mainTouch(const TouchEvt &e) {
    if (e.kind != TouchEvt::UP) return;
    if (notice_.active) { endNotice(); return; }
    if (!uiVisible_) { uiVisible_ = true; dirty_ = true; return; }
    if (inRect(e.x, e.y, 0, 0, 52, 52)) { openSettings(0); return; }
    if (inRect(e.x, e.y, W - 52, 0, 52, 52)) { doPower(); return; }
    uiVisible_ = false;     // tap on an empty area hides the buttons
    dirty_ = true;
}

// ------------------------------------------------------------------------------------------
// Settings
// ------------------------------------------------------------------------------------------
void App::commit() { settingsSave(s); }

App::Row &App::addRow(RowKind kind, const char *label, int h) {
    rows_.emplace_back();
    Row &r = rows_.back();
    r.kind = kind;
    snprintf(r.label, sizeof(r.label), "%s", label);
    r.h = h;
    return r;
}

void App::addToggle(const char *label, uint8_t *p) {
    Row &r = addRow(RK_TOGGLE, label, 40);
    r.get = [p] { return (int)*p; };
    r.set = [p](int v) { *p = (uint8_t)(v ? 1 : 0); };
}

void App::addChoice(const char *label, uint8_t *p, const char *const *names, int n) {
    Row &r = addRow(RK_CHOICE, label, 40);
    r.get = [p] { return (int)*p; };
    r.set = [p](int v) { *p = (uint8_t)v; };
    r.names = names;
    r.nNames = n;
}

void App::addSlider(const char *label, std::function<int()> get, std::function<void(int)> set, int mn, int mx,
                    std::function<void(char *, int)> fmt) {
    Row &r = addRow(RK_SLIDER, label, 56);
    r.get = std::move(get);
    r.set = std::move(set);
    r.mn = mn;
    r.mx = mx;
    r.fmt = std::move(fmt);
}

void App::addSwatch(const char *label, const uint16_t *palette, uint16_t *p) {
    Row &r = addRow(RK_SWATCH, label, 64);
    r.palette = palette;
    r.get = [palette, p] {
        for (int i = 0; i < 8; ++i) if (palette[i] == *p) return i;
        return -1;
    };
    r.set = [palette, p](int i) { if (i >= 0 && i < 8) *p = palette[i]; };
}

void App::addButton(const char *label, std::function<void()> onTap) {
    Row &r = addRow(RK_BUTTON, label, 46);
    r.onTap = std::move(onTap);
}

void App::addLabel(const char *text) { addRow(RK_LABEL, text, 30); }

void App::rowsGeneral() {
    addSlider("Độ sáng đèn nền", [this] { return (int)s.backlight; },
              [this](int v) { s.backlight = (uint8_t)v; hal_backlight(s.backlight); }, 10, 255,
              [](char *b, int v) { snprintf(b, 16, "%d%%", v * 100 / 255); });
    addSlider("Độ sáng nền", [this] { return (int)s.bgBrightness; }, [this](int v) { s.bgBrightness = (uint8_t)v; }, 0, 100,
              [](char *b, int v) { snprintf(b, 16, "%d%%", v); });
    addToggle("Tự đổi màu nền theo giờ", &s.autoSky);
    addLabel("Bình minh 6h • ngày 9h • hoàng hôn 18h • đêm 21h");
    addSwatch("Màu nền cố định (khi tắt tự đổi)", kBgPalette, &s.bgColor);
    addChoice("Nút nguồn", &s.powerMode, kPowerNames, 2);
    Row &hide = addRow(RK_STEPPER, "Tự ẩn nút sau (giây)", 40);
    hide.get = [this] { return (int)s.autoHideSec; };
    hide.set = [this](int v) { s.autoHideSec = (uint8_t)v; };
    hide.mn = 0; hide.mx = 60; hide.step = 2;
    hide.fmt = [](char *b, int v) { if (v == 0) snprintf(b, 16, "Tắt"); else snprintf(b, 16, "%d", v); };
    Row &flip = addRow(RK_TOGGLE, "Xoay màn hình 180°", 40);
    flip.get = [this] { return (int)s.flipped; };
    flip.set = [this](int v) {
        s.flipped = (uint8_t)v;
        hal_set_flipped(s.flipped);
        s.cal.valid = 0;           // touch mapping changes with the picture
        commit();
        startCalibration();
    };
    addButton("Hiệu chỉnh cảm ứng", [this] { startCalibration(); });
    addButton(confirmReset_ ? "Chạm lần nữa để xác nhận" : "Khôi phục cài đặt gốc", [this] {
        if (!confirmReset_) { confirmReset_ = true; return; }
        confirmReset_ = false;
        Calibration keep = s.cal;
        settingsDefaults(s);
        s.cal = keep;
        hal_backlight(s.backlight);
        commit();
    });
    addLabel("CYD Clock 0.1  (ESP32-2432S028)");
}

void App::rowsConnect() {
    char line[56];
    char cur[34] = "";
    const int st = hal_wifi_status();
    hal_wifi_ssid(cur, sizeof(cur));
    if (st == WIFI_CONNECTED) snprintf(line, sizeof(line), "WiFi: đã nối %s", cur);
    else if (st == WIFI_CONNECTING) snprintf(line, sizeof(line), "WiFi: đang kết nối...");
    else if (st == WIFI_FAILED) snprintf(line, sizeof(line), "WiFi: kết nối thất bại");
    else snprintf(line, sizeof(line), "WiFi: chưa kết nối");
    addLabel(line);
    addButton(scanning_ ? "Đang quét..." : "Quét WiFi", [this] {
        if (scanning_) return;
        scanning_ = true;
        hal_wifi_scan_start();
    });
    for (const WifiAp &ap : aps_) {
        Row &r = addRow(RK_NET, ap.ssid, 38);
        r.secure = ap.secure;
        r.rssi = ap.rssi;
        r.connected = (st == WIFI_CONNECTED && strcmp(cur, ap.ssid) == 0);
        const WifiAp copy = ap;
        r.onTap = [this, copy] {
            if (!copy.secure) {
                snprintf(s.ssid, sizeof(s.ssid), "%s", copy.ssid);
                s.pass[0] = 0;
                commit();
                hal_wifi_connect(s.ssid, "");
                ntpRequested_ = false;
                return;
            }
            char title[56];
            snprintf(title, sizeof(title), "Mật khẩu cho %s", copy.ssid);
            const bool same = strcmp(s.ssid, copy.ssid) == 0;
            std::string ssid = copy.ssid;
            openKeyboard(title, same ? s.pass : "", true, [this, ssid](const char *pw) {
                snprintf(s.ssid, sizeof(s.ssid), "%s", ssid.c_str());
                snprintf(s.pass, sizeof(s.pass), "%s", pw);
                commit();
                hal_wifi_connect(s.ssid, s.pass);
                ntpRequested_ = false;
            });
        };
    }
    Row &tz = addRow(RK_STEPPER, "Múi giờ (GMT)", 40);
    tz.get = [this] { return (int)s.tzHalfHours; };
    tz.set = [this](int v) { s.tzHalfHours = (int8_t)v; };
    tz.mn = -24; tz.mx = 28; tz.step = 1;
    tz.fmt = [](char *b, int v) {
        const int a = v < 0 ? -v : v;
        snprintf(b, 16, "%c%d%s", v < 0 ? '-' : '+', a / 2, (a % 2) ? ":30" : "");
    };
    addToggle("Tự kết nối WiFi khi bật nguồn", &s.wifiAuto);
    addButton("Đồng bộ giờ ngay (NTP)", [this] { ntpRequested_ = false; hal_ntp_request(); });
    addLabel(timeOk() ? "Giờ: đã đồng bộ" : "Giờ: chưa đồng bộ");
    addToggle("Lấy giờ từ điện thoại qua BLE (sắp có)", &s.bleTime);
    addButton("Quên WiFi đã lưu", [this] {
        s.ssid[0] = 0;
        s.pass[0] = 0;
        commit();
        hal_wifi_disconnect();
    });
}

void App::rowsClock() {
    static const char *names[20];
    int n = 0;
    for (int i = 0; i < kDigitFontCount && n < 17; ++i) names[n++] = kDigitFonts[i]->name;
    names[n++] = "LED 7 đoạn";
    names[n++] = "Digital-7 (nghiêng)";
    addChoice("Kiểu số", &s.clockFont, names, n);
    const int cap = maxClockHeight(s.clockFont, W, 4);
    addSlider("Cỡ số", [this, cap] { return std::min<int>(s.clockSize, cap); },
              [this](int v) { s.clockSize = (uint8_t)v; }, 30, cap < 31 ? 31 : cap,
              [](char *b, int v) { snprintf(b, 16, "%d px", v); });
    addButton("Chỉnh vị trí bằng mũi tên", [this] { openAdjust(); });
    char pos[56];
    snprintf(pos, sizeof(pos), "Vị trí: X = %d, Y = %d", s.clockX, s.clockY);
    addLabel(pos);
    addToggle("Định dạng 24 giờ", &s.hour24);
    addToggle("Hiện số 0 đầu giờ", &s.leadingZero);
    addToggle("Dấu hai chấm nhấp nháy", &s.colonBlink);
    addChoice("Hiệu ứng đồng hồ", &s.clockEffect, kEffectNames, 4);
    addSwatch("Màu số", kTextPalette, &s.clockColor);
    addToggle("Hiện ngày tháng", &s.showDate);
    addChoice("Cỡ chữ ngày", &s.dateSize, kDateSizeNames, 4);
    addChoice("Kiểu chữ ngày", &s.dateFont, kDateFontNames, kDateFontCount);
}

void App::rowsNotify() {
    addToggle("Hiện popup giữa màn hình", &s.ntfPopup);
    addToggle("Hiện thẻ chạy dòng phía dưới", &s.ntfTicker);
    addChoice("Cỡ chữ thông báo", &s.ntfSize, kNtfSizeNames, 3);
    addSwatch("Màu nền thẻ", kCardPalette, &s.ntfBg);
    addSwatch("Màu chữ", kTextPalette, &s.ntfFg);
    addChoice("Hiệu ứng khi có thông báo", &s.ntfEffect, kNtfEffectNames, 4);
    addSlider("Thời gian hiện", [this] { return (int)s.ntfSeconds; }, [this](int v) { s.ntfSeconds = (uint8_t)v; }, 3, 20,
              [](char *b, int v) { snprintf(b, 16, "%d giây", v); });
    addButton("Xem trước", [this] {
        commit();
        returnToSettings_ = true;
        pushNotice("Zalo", "Anh Nam: tối nay đi ăn ở đâu nhỉ? Mình đặt bàn lúc 7 giờ nhé", true);
    });
    addLabel("Nguồn thông báo: app trên điện thoại qua BLE (sắp có)");
}

void App::buildRows() {
    rows_.clear();
    switch (tab_) {
        case 0: rowsGeneral(); break;
        case 1: rowsConnect(); break;
        case 2: rowsClock(); break;
        default: rowsNotify(); break;
    }
    int y = 0;
    for (Row &r : rows_) { r.y = y; y += r.h; }
    const int maxScroll = std::max(0, contentHeight() - (H - 32));
    scroll_ = std::min(scroll_, maxScroll);
    if (scroll_ < 0) scroll_ = 0;
}

int App::contentHeight() const {
    int h = 0;
    for (const Row &r : rows_) h += r.h;
    return h;
}

int App::rowAt(int yContent) const {
    for (size_t i = 0; i < rows_.size(); ++i)
        if (yContent >= rows_[i].y && yContent < rows_[i].y + rows_[i].h) return (int)i;
    return -1;
}

void App::openSettings(int tab) {
    screen_ = SCR_SETTINGS;
    tab_ = tab;
    scroll_ = 0;
    confirmReset_ = false;
    if (tab == 1 && !scanning_ && aps_.empty()) { scanning_ = true; hal_wifi_scan_start(); }
    buildRows();
    drawSettingsFull();
}

void App::drawTabs() {
    Gfx &g = gfx();
    g.fillRect(0, 0, W, 32, C_BG);
    for (int i = 0; i < 4; ++i) {
        const bool act = i == tab_;
        const int x = tabX()[i], w = tabX()[i + 1] - tabX()[i];
        const uint16_t fill = act ? C_ACC : C_BTN;
        g.fillRoundRect(x + 2, 3, w - 4, 27, 7, fill);
        drawTextCentered(TF_R14, x + w / 2, 3 + (27 - textHeight(TF_R14)) / 2, kTabNames[i],
                         act ? rgb(255, 255, 255) : C_MUT, Bg::solid(fill));
    }
    const uint16_t red = rgb(130, 44, 52);
    g.fillRoundRect(tabX()[4] + 2, 3, W - tabX()[4] - 4, 27, 7, red);
    const int cx = (tabX()[4] + W) / 2, cy = 16;
    for (int d = -1; d <= 1; ++d) {      // a 3 px thick "X"
        g.drawLine(cx - 6 + d, cy - 6, cx + 6 + d, cy + 6, rgb(255, 255, 255));
        g.drawLine(cx - 6 + d, cy + 6, cx + 6 + d, cy - 6, rgb(255, 255, 255));
    }
}

void App::drawRow(const Row &r, int top) {
    Gfx &g = gfx();
    const int bh = r.h - 2;
    const uint16_t rowBg = C_ROW;
    const Bg bg = Bg::solid(rowBg);
    g.fillRect(0, top, W, bh, rowBg);
    g.fillRect(0, top + bh, W, 2, C_BG);
    const int cy = top + bh / 2;
    char val[24] = "";
    switch (r.kind) {
        case RK_LABEL: {
            char t[64];
            snprintf(t, sizeof(t), "%s", r.label);
            fitText(TF_R12, t, W - 20);
            drawText(TF_R12, 10, cy - textHeight(TF_R12) / 2, t, C_MUT, bg);
            break;
        }
        case RK_TOGGLE: {
            char t[64];
            snprintf(t, sizeof(t), "%s", r.label);
            fitText(TF_R14, t, W - 90);
            drawText(TF_R14, 10, cy - textHeight(TF_R14) / 2, t, C_TXT, bg);
            const bool on = r.get() != 0;
            g.fillRoundRect(W - 62, cy - 12, 50, 24, 12, on ? C_ACC : C_TRACK);
            g.fillCircle(on ? W - 25 : W - 49, cy, 9, rgb(255, 255, 255));
            break;
        }
        case RK_CHOICE: {
            const int v = r.get();
            snprintf(val, sizeof(val), "%s", (v >= 0 && v < r.nNames) ? r.names[v] : "?");
            const int vw = textWidth(TF_B14, val);
            char t[64];
            snprintf(t, sizeof(t), "%s", r.label);
            fitText(TF_R14, t, W - 40 - vw - 12);
            drawText(TF_R14, 10, cy - textHeight(TF_R14) / 2, t, C_TXT, bg);
            drawText(TF_B14, W - 28 - vw, cy - textHeight(TF_B14) / 2, val, C_ACC, bg);
            drawArrow(W - 14, cy, 1, 5, C_MUT);
            break;
        }
        case RK_STEPPER: {
            const int v = r.get();
            if (r.fmt) r.fmt(val, v); else snprintf(val, sizeof(val), "%d", v);
            char t[64];
            snprintf(t, sizeof(t), "%s", r.label);
            fitText(TF_R14, t, W - 150);
            drawText(TF_R14, 10, cy - textHeight(TF_R14) / 2, t, C_TXT, bg);
            g.fillRoundRect(W - 134, cy - 14, 30, 28, 7, C_BTN);
            g.fillRect(W - 126, cy - 1, 14, 3, C_TXT);
            g.fillRoundRect(W - 44, cy - 14, 30, 28, 7, C_BTN);
            g.fillRect(W - 36, cy - 1, 14, 3, C_TXT);
            g.fillRect(W - 30, cy - 7, 3, 14, C_TXT);
            drawTextCentered(TF_B14, W - 74, cy - textHeight(TF_B14) / 2, val, C_ACC, bg);
            break;
        }
        case RK_SLIDER: {
            const int v = r.get();
            if (r.fmt) r.fmt(val, v); else snprintf(val, sizeof(val), "%d", v);
            drawText(TF_R14, 10, top + 4, r.label, C_TXT, bg);
            drawText(TF_B14, W - 12 - textWidth(TF_B14, val), top + 4, val, C_ACC, bg);
            const int tx = 16, tw = W - 32, ty = top + 38;
            const int span = r.mx - r.mn > 0 ? r.mx - r.mn : 1;
            const int kx = tx + (v - r.mn) * tw / span;
            g.fillRoundRect(tx, ty - 3, tw, 6, 3, C_TRACK);
            g.fillRoundRect(tx, ty - 3, kx - tx + 1, 6, 3, C_ACC);
            g.fillCircle(kx, ty, 9, rgb(255, 255, 255));
            break;
        }
        case RK_BUTTON: {
            const uint16_t fill = r.enabled ? C_BTN : C_ROW2;
            g.fillRoundRect(10, top + 3, W - 20, bh - 6, 9, fill);
            g.drawRoundRect(10, top + 3, W - 20, bh - 6, 9, C_ACC);
            drawTextCentered(TF_B14, W / 2, cy - textHeight(TF_B14) / 2, r.label, r.enabled ? C_TXT : C_MUT, Bg::solid(fill));
            break;
        }
        case RK_SWATCH: {
            char t[64];
            snprintf(t, sizeof(t), "%s", r.label);
            fitText(TF_R14, t, W - 20);
            drawText(TF_R14, 10, top + 4, t, C_TXT, bg);
            const int sel = r.get();
            for (int i = 0; i < 8; ++i) {
                const int x = 10 + i * 37;
                g.fillRoundRect(x, top + 30, 33, 26, 5, r.palette[i]);
                g.drawRoundRect(x, top + 30, 33, 26, 5, i == sel ? rgb(255, 255, 255) : C_TRACK);
                if (i == sel) g.drawRoundRect(x + 1, top + 31, 31, 24, 4, rgb(255, 255, 255));
            }
            break;
        }
        case RK_NET: {
            char t[40];
            snprintf(t, sizeof(t), "%s", r.label);
            fitText(TF_R14, t, 196);
            drawText(TF_R14, 12, cy - textHeight(TF_R14) / 2, t, r.connected ? C_OK : C_TXT, bg);
            const int level = r.rssi > -55 ? 4 : (r.rssi > -65 ? 3 : (r.rssi > -75 ? 2 : 1));
            for (int k = 0; k < 4; ++k) {
                const int bh2 = 4 + 3 * k;
                g.fillRect(222 + k * 6, cy + 8 - bh2, 4, bh2, k < level ? C_TXT : C_TRACK);
            }
            if (r.secure) {
                g.fillRect(262, cy - 2, 12, 9, C_MUT);
                g.drawCircle(268, cy - 3, 4, C_MUT);
            }
            if (r.connected) {
                g.drawLine(288, cy, 292, cy + 5, C_OK);
                g.drawLine(292, cy + 5, 300, cy - 5, C_OK);
                g.drawLine(288, cy + 1, 292, cy + 6, C_OK);
                g.drawLine(292, cy + 6, 300, cy - 4, C_OK);
            }
            break;
        }
    }
}

void App::drawRows() {
    const int endY = 32 + contentHeight() - scroll_;
    if (endY < H) gfx().fillRect(0, std::max(endY, 32), W, H - std::max(endY, 32), C_BG);
    for (const Row &r : rows_) {
        const int top = 32 + r.y - scroll_;
        if (top + r.h <= 32 || top >= H) continue;
        drawRow(r, top);
    }
    const int ch = contentHeight();
    const int vh = H - 32;
    if (ch > vh) {
        const int barH = std::max(16, vh * vh / ch);
        const int barY = 32 + (vh - barH) * scroll_ / (ch - vh);
        gfx().fillRoundRect(W - 3, barY, 3, barH, 1, rgb(90, 100, 130));
    }
    drawTabs();
}

void App::drawSettingsFull() {
    gfx().fillRect(0, 0, W, H, C_BG);
    drawRows();
}

void App::setSliderFromX(Row &r, int x) {
    const int tx = 16, tw = W - 32;
    int v = r.mn + (x - tx) * (r.mx - r.mn) / tw;
    v = std::max(r.mn, std::min(r.mx, v));
    if (v != r.get()) r.set(v);
}

void App::settingsTouch(const TouchEvt &e) {
    if (e.kind == TouchEvt::DOWN) {
        downX_ = e.x; downY_ = e.y;
        dragging_ = false;
        tabPress_ = e.y < 32;
        downRow_ = -1;
        activeSlider_ = -1;
        if (tabPress_) {
            tabDown_ = -1;
            for (int i = 0; i < 5; ++i) if (e.x >= tabX()[i] && (i == 4 || e.x < tabX()[i + 1])) tabDown_ = i;
            return;
        }
        scrollStart_ = scroll_;
        const int idx = rowAt(e.y - 32 + scroll_);
        downRow_ = idx;
        if (idx >= 0 && rows_[idx].kind == RK_SLIDER) {
            activeSlider_ = idx;
            setSliderFromX(rows_[idx], e.x);
            drawRow(rows_[idx], 32 + rows_[idx].y - scroll_);
            drawTabs();
        }
        return;
    }
    if (e.kind == TouchEvt::MOVE) {
        if (tabPress_) return;
        if (activeSlider_ >= 0) {
            Row &r = rows_[activeSlider_];
            setSliderFromX(r, e.x);
            drawRow(r, 32 + r.y - scroll_);
            drawTabs();
            return;
        }
        if (abs(e.y - downY_) > 8) dragging_ = true;
        if (dragging_) {
            const int maxScroll = std::max(0, contentHeight() - (H - 32));
            const int ns = std::max(0, std::min(maxScroll, scrollStart_ + (downY_ - e.y)));
            const uint32_t ms = hal_millis();
            if (ns != scroll_ && ms - lastScrollDraw_ >= 30) {
                scroll_ = ns;
                lastScrollDraw_ = ms;
                drawRows();
            }
        }
        return;
    }
    // UP
    if (tabPress_) {
        tabPress_ = false;
        int hit = -1;
        for (int i = 0; i < 5; ++i) if (e.x >= tabX()[i] && (i == 4 || e.x < tabX()[i + 1])) hit = i;
        if (e.y < 32 && hit == tabDown_ && hit >= 0) {
            if (hit == 4) { commit(); backToMain(); return; }
            if (hit != tab_) openSettings(hit);
        }
        return;
    }
    if (activeSlider_ >= 0) {
        activeSlider_ = -1;
        commit();
        return;
    }
    if (!dragging_ && downRow_ >= 0) {
        const int idx = rowAt(e.y - 32 + scroll_);
        if (idx == downRow_) activateRow(idx, e.x);
    }
    downRow_ = -1;
}

void App::activateRow(int index, int x) {
    Row &r = rows_[index];
    switch (r.kind) {
        case RK_TOGGLE: r.set(r.get() ? 0 : 1); break;
        case RK_CHOICE: r.set((r.get() + 1) % (r.nNames > 0 ? r.nNames : 1)); break;
        case RK_STEPPER: {
            int v = r.get() + (x < W - 74 ? -r.step : r.step);
            v = std::max(r.mn, std::min(r.mx, v));
            r.set(v);
            break;
        }
        case RK_BUTTON:
        case RK_NET:
            if (r.enabled && r.onTap) r.onTap();
            break;
        case RK_SWATCH: {
            const int i = (x - 10) / 37;
            if (i >= 0 && i < 8 && (x - 10) % 37 < 33) r.set(i);
            break;
        }
        default: break;
    }
    if (screen_ != SCR_SETTINGS) return;      // the action opened another screen
    commit();
    buildRows();
    drawRows();
}

void App::pollConnect() {
    if (tab_ != 1) return;
    bool changed = false;
    if (scanning_) {
        const int n = hal_wifi_scan_state();
        if (n >= 0) {
            scanning_ = false;
            aps_.clear();
            for (int i = 0; i < n && aps_.size() < 12; ++i) {
                WifiAp ap;
                if (!hal_wifi_scan_get(i, ap) || ap.ssid[0] == 0) continue;
                bool dup = false;
                for (WifiAp &o : aps_) if (strcmp(o.ssid, ap.ssid) == 0) { dup = true; if (ap.rssi > o.rssi) o = ap; }
                if (!dup) aps_.push_back(ap);
            }
            std::sort(aps_.begin(), aps_.end(), [](const WifiAp &a, const WifiAp &b) { return a.rssi > b.rssi; });
            changed = true;
        }
    }
    const int st = hal_wifi_status();
    if (st != wifiPrev_) { wifiPrev_ = st; changed = true; }
    if (changed && !dragging_ && activeSlider_ < 0) {
        buildRows();
        drawRows();
    }
}

// ------------------------------------------------------------------------------------------
// Position / size adjust screen
// ------------------------------------------------------------------------------------------
namespace {
struct Pad { int x, y, w, h; };
const Pad kAdjUp = {242, 148, 36, 36}, kAdjLeft = {200, 190, 36, 36}, kAdjDown = {242, 190, 36, 36},
          kAdjRight = {284, 190, 36, 36}, kAdjMinus = {8, 190, 48, 36}, kAdjPlus = {60, 190, 48, 36},
          kAdjReset = {4, 4, 76, 30}, kAdjDone = {W - 80, 4, 76, 30};
}  // namespace

void App::openAdjust() {
    commit();
    screen_ = SCR_ADJUST;
    adjustHold_ = -1;
    drawAdjustFull();
}

void App::drawAdjustFull() {
    const bool vis = uiVisible_;
    uiVisible_ = false;
    computeBg();
    drawMainFull();
    uiVisible_ = vis;
    const uint16_t pad = rgb(30, 40, 66);
    const uint16_t white = rgb(255, 255, 255);
    gfx().fillRoundRect(kAdjUp.x, kAdjUp.y, kAdjUp.w, kAdjUp.h, 8, pad);
    drawArrow(kAdjUp.x + 18, kAdjUp.y + 18, 0, 8, white);
    gfx().fillRoundRect(kAdjLeft.x, kAdjLeft.y, kAdjLeft.w, kAdjLeft.h, 8, pad);
    drawArrow(kAdjLeft.x + 18, kAdjLeft.y + 18, 3, 8, white);
    gfx().fillRoundRect(kAdjDown.x, kAdjDown.y, kAdjDown.w, kAdjDown.h, 8, pad);
    drawArrow(kAdjDown.x + 18, kAdjDown.y + 18, 2, 8, white);
    gfx().fillRoundRect(kAdjRight.x, kAdjRight.y, kAdjRight.w, kAdjRight.h, 8, pad);
    drawArrow(kAdjRight.x + 18, kAdjRight.y + 18, 1, 8, white);
    button(kAdjMinus.x, kAdjMinus.y, kAdjMinus.w, kAdjMinus.h, "A-", pad, white, TF_B18);
    button(kAdjPlus.x, kAdjPlus.y, kAdjPlus.w, kAdjPlus.h, "A+", pad, white, TF_B18);
    button(kAdjReset.x, kAdjReset.y, kAdjReset.w, kAdjReset.h, "Đặt lại", pad, white, TF_B14);
    button(kAdjDone.x, kAdjDone.y, kAdjDone.w, kAdjDone.h, "Xong", C_ACC, white, TF_B14);
    char t[48];
    snprintf(t, sizeof(t), "X %d   Y %d   Cỡ %d", s.clockX, s.clockY, s.clockSize);
    drawTextCentered(TF_R12, W / 2, 10, t, rgb(255, 255, 255), bg_);
}

void App::adjustStep(int code) {
    const int cap = maxClockHeight(s.clockFont, W, 4);
    switch (code) {
        case 0: s.clockY = (int16_t)std::max(-120, s.clockY - 2); break;
        case 1: s.clockX = (int16_t)std::min(160, s.clockX + 2); break;
        case 2: s.clockY = (int16_t)std::min(120, s.clockY + 2); break;
        case 3: s.clockX = (int16_t)std::max(-160, s.clockX - 2); break;
        case 4: s.clockSize = (uint8_t)std::max(30, (int)s.clockSize - 2); break;
        case 5: s.clockSize = (uint8_t)std::min(cap, (int)s.clockSize + 2); break;
        default: break;
    }
    drawAdjustFull();
}

void App::adjustTouch(const TouchEvt &e) {
    struct Hit { const Pad *p; int code; };
    const Hit hits[8] = {{&kAdjUp, 0}, {&kAdjRight, 1}, {&kAdjDown, 2}, {&kAdjLeft, 3},
                         {&kAdjMinus, 4}, {&kAdjPlus, 5}, {&kAdjReset, 6}, {&kAdjDone, 7}};
    int code = -1;
    for (const Hit &h : hits) if (inRect(e.x, e.y, h.p->x, h.p->y, h.p->w, h.p->h)) code = h.code;
    if (e.kind == TouchEvt::DOWN) {
        if (code >= 0 && code <= 5) {
            adjustStep(code);
            adjustHold_ = code;
            adjustNext_ = hal_millis() + 400;
        }
        return;
    }
    if (e.kind == TouchEvt::MOVE) return;
    adjustHold_ = -1;
    if (code == 6) { s.clockX = 0; s.clockY = 0; drawAdjustFull(); }
    else if (code == 7) { commit(); openSettings(2); }
}

// ------------------------------------------------------------------------------------------
// On-screen keyboard
// ------------------------------------------------------------------------------------------
namespace {
struct Key { int x, y, w, h; char label[8]; int code; };

int buildKeys(Key *k, bool shift, bool symbols, bool show) {
    int n = 0;
    auto add = [&](int x, int y, int w, int code, const char *label) {
        k[n].x = x; k[n].y = y; k[n].w = w; k[n].h = 34; k[n].code = code;
        snprintf(k[n].label, sizeof(k[n].label), "%s", label);
        ++n;
    };
    const char *digits = "1234567890";
    const char *r2 = symbols ? (shift ? "\\|~<>`_;:^" : "-/:;()$&@\"") : "qwertyuiop";
    const char *r3 = symbols ? (shift ? "!?#%&*'\"/" : ".,?!'#%^*") : "asdfghjkl";
    const char *r4 = symbols ? (shift ? "()[]{}=" : "[]{}+=_") : "zxcvbnm";
    for (int i = 0; i < 10; ++i) { char c[2] = {digits[i], 0}; add(i * 32, 58, 32, digits[i], c); }
    for (int i = 0; i < 10; ++i) {
        char c[2] = {r2[i], 0};
        const int code = (!symbols && shift) ? (r2[i] - 32) : r2[i];
        if (!symbols && shift) c[0] = (char)code;
        add(i * 32, 94, 32, code, c);
    }
    for (int i = 0; i < 9; ++i) {
        char c[2] = {r3[i], 0};
        const int code = (!symbols && shift) ? (r3[i] - 32) : r3[i];
        if (!symbols && shift) c[0] = (char)code;
        add(16 + i * 32, 130, 32, code, c);
    }
    add(0, 166, 48, 1, symbols ? "=\\<" : "^");
    for (int i = 0; i < 7; ++i) {
        char c[2] = {r4[i], 0};
        const int code = (!symbols && shift) ? (r4[i] - 32) : r4[i];
        if (!symbols && shift) c[0] = (char)code;
        add(48 + i * 32, 166, 32, code, c);
    }
    add(272, 166, 48, 2, "<-");
    add(0, 202, 56, 3, symbols ? "abc" : "?123");
    add(56, 202, 152, ' ', "");
    add(208, 202, 56, 5, show ? "Ẩn" : "Hiện");
    add(264, 202, 56, 6, "OK");
    return n;
}
}  // namespace

void App::openKeyboard(const char *title, const char *initial, bool secret, std::function<void(const char *)> done) {
    snprintf(kbTitle_, sizeof(kbTitle_), "%s", title);
    snprintf(kbText_, sizeof(kbText_), "%s", initial);
    kbSecret_ = secret;
    kbShow_ = false;
    kbShift_ = false;
    kbSymbols_ = false;
    kbDone_ = std::move(done);
    screen_ = SCR_KEYBOARD;
    drawKeyboard();
}

void App::drawKeyboard() {
    Gfx &g = gfx();
    g.fillRect(0, 0, W, H, C_BG);
    char t[56];
    snprintf(t, sizeof(t), "%s", kbTitle_);
    fitText(TF_R12, t, 240);
    drawText(TF_R12, 8, 4, t, C_MUT, Bg::solid(C_BG));
    button(262, 2, 52, 20, "Hủy", rgb(110, 40, 48), rgb(255, 255, 255), TF_R12);
    g.fillRoundRect(6, 24, W - 12, 30, 7, rgb(14, 18, 30));
    g.drawRoundRect(6, 24, W - 12, 30, 7, C_ACC);
    char shown[130] = "";
    const int len = (int)strlen(kbText_);
    if (kbSecret_ && !kbShow_) { for (int i = 0; i < len && i < 40; ++i) strcat(shown, "•"); }
    else snprintf(shown, sizeof(shown), "%s", kbText_);
    // keep the tail visible when the text is wider than the field
    const char *p = shown;
    while (*p && textWidth(TF_R18, p) > W - 36) ++p;
    drawText(TF_R18, 14, 24 + (30 - textHeight(TF_R18)) / 2, p, rgb(255, 255, 255), Bg::solid(rgb(14, 18, 30)));
    Key keys[48];
    const int n = buildKeys(keys, kbShift_, kbSymbols_, kbShow_);
    for (int i = 0; i < n; ++i) {
        const Key &k = keys[i];
        uint16_t fill = C_BTN;
        if (k.code == 6) fill = C_ACC;
        else if (k.code == 1 && kbShift_) fill = rgb(70, 110, 190);
        else if (k.code == 2 || k.code == 3 || k.code == 5 || k.code == 1) fill = rgb(46, 56, 84);
        g.fillRoundRect(k.x + 2, k.y + 1, k.w - 4, k.h - 2, 6, fill);
        if (k.label[0]) drawTextCentered(TF_B14, k.x + k.w / 2, k.y + (k.h - textHeight(TF_B14)) / 2, k.label, rgb(255, 255, 255), Bg::solid(fill));
    }
}

void App::keyboardTouch(const TouchEvt &e) {
    if (e.kind != TouchEvt::UP) return;
    if (inRect(e.x, e.y, 262, 2, 52, 20)) { screen_ = SCR_SETTINGS; buildRows(); drawSettingsFull(); return; }
    Key keys[48];
    const int n = buildKeys(keys, kbShift_, kbSymbols_, kbShow_);
    for (int i = 0; i < n; ++i) {
        const Key &k = keys[i];
        if (!inRect(e.x, e.y, k.x, k.y, k.w, k.h)) continue;
        const int len = (int)strlen(kbText_);
        switch (k.code) {
            case 1: kbShift_ = !kbShift_; break;
            case 2: if (len > 0) kbText_[len - 1] = 0; break;
            case 3: kbSymbols_ = !kbSymbols_; kbShift_ = false; break;
            case 5: kbShow_ = !kbShow_; break;
            case 6: {
                char out[66];
                snprintf(out, sizeof(out), "%s", kbText_);
                screen_ = SCR_SETTINGS;
                if (kbDone_) kbDone_(out);
                buildRows();
                drawSettingsFull();
                return;
            }
            default:
                if (len < 64) { kbText_[len] = (char)k.code; kbText_[len + 1] = 0; }
                if (kbShift_ && !kbSymbols_) kbShift_ = false;
                break;
        }
        drawKeyboard();
        return;
    }
}

// ------------------------------------------------------------------------------------------
// Touch calibration
// ------------------------------------------------------------------------------------------
namespace {
const int kCalX[4] = {20, 299, 20, 299};
const int kCalY[4] = {20, 20, 219, 219};

void crosshair(int x, int y, uint16_t c) {
    gfx().drawLine(x - 14, y, x + 14, y, c);
    gfx().drawLine(x, y - 14, x, y + 14, c);
    gfx().drawCircle(x, y, 8, c);
}
}  // namespace

void App::startCalibration() {
    screen_ = SCR_CALIB;
    calStep_ = 0;
    calTest_ = false;
    calN_ = 0;
    calSkip_ = 0;
    calSumX_ = calSumY_ = 0;
    calMsg_ = 0;
    pressed_ = false;
    stable_ = 0;
    ignoreUntilUp_ = false;
    drawCalib();
}

void App::drawCalib() {
    Gfx &g = gfx();
    g.fillRect(0, 0, W, H, rgb(0, 0, 0));
    const Bg black = Bg::solid(0);
    if (!calTest_) {
        crosshair(kCalX[calStep_], kCalY[calStep_], rgb(255, 214, 10));
        char t[40];
        snprintf(t, sizeof(t), "Chạm đúng giữa dấu + (%d/4)", calStep_ + 1);
        drawTextCentered(TF_B18, W / 2, 96, t, rgb(255, 255, 255), black);
        drawTextCentered(TF_R14, W / 2, 124, "Giữ ngón tay hoặc bút đầu mềm trong 1 giây", rgb(170, 176, 190), black);
        if (calMsg_) drawTextCentered(TF_B14, W / 2, 150, "Chưa đúng, hãy chạm lại từ dấu đầu tiên", C_BAD, black);
    } else {
        drawTextCentered(TF_B18, W / 2, 40, "Chạm thử", rgb(255, 255, 255), black);
        drawTextCentered(TF_R14, W / 2, 68, "Chấm xanh phải nằm đúng dưới ngón tay", rgb(170, 176, 190), black);
        button(20, 196, 130, 36, "Lưu", C_OK, rgb(0, 0, 0), TF_B18);
        button(170, 196, 130, 36, "Làm lại", C_BTN, rgb(255, 255, 255), TF_B18);
    }
}

void App::calibRaw(bool pressed, int rx, int ry) {
    if (pressed) {
        if (calSkip_ < 3) { ++calSkip_; return; }    // ignore the first, noisy samples
        if (calN_ % 4 == 0) {
            char t[40];
            snprintf(t, sizeof(t), "raw %4d, %4d", rx, ry);
            gfx().fillRect(0, 222, W, 18, rgb(0, 0, 0));
            drawText(TF_R12, 8, 223, t, rgb(110, 120, 140), Bg::solid(0));
        }
        calSumX_ += rx;
        calSumY_ += ry;
        ++calN_;
        return;
    }
    if (calN_ >= 5) {
        calRaw_[calStep_][0] = (int)(calSumX_ / calN_);
        calRaw_[calStep_][1] = (int)(calSumY_ / calN_);
        ++calStep_;
        calMsg_ = 0;
        if (calStep_ == 4) {
            const int dxX = abs(calRaw_[1][0] - calRaw_[0][0]) + abs(calRaw_[3][0] - calRaw_[2][0]);
            const int dxY = abs(calRaw_[1][1] - calRaw_[0][1]) + abs(calRaw_[3][1] - calRaw_[2][1]);
            const bool swap = dxY > dxX;
            auto A = [&](int i) { return swap ? calRaw_[i][1] : calRaw_[i][0]; };
            auto B = [&](int i) { return swap ? calRaw_[i][0] : calRaw_[i][1]; };
            Calibration c{};
            c.swap = swap ? 1 : 0;
            c.a0 = (int16_t)((A(0) + A(2)) / 2);
            c.a1 = (int16_t)((A(1) + A(3)) / 2);
            c.b0 = (int16_t)((B(0) + B(1)) / 2);
            c.b1 = (int16_t)((B(2) + B(3)) / 2);
            if (abs(c.a1 - c.a0) > 400 && abs(c.b1 - c.b0) > 400) {
                c.valid = 1;
                calTmp_ = c;
                calTest_ = true;
            } else {
                calStep_ = 0;
                calMsg_ = 1;
            }
        }
        drawCalib();
    }
    calN_ = 0;
    calSkip_ = 0;
    calSumX_ = calSumY_ = 0;
}

void App::calibTouch(const TouchEvt &e) {
    if (!calTest_) return;
    if (e.kind == TouchEvt::UP) {
        if (inRect(e.x, e.y, 20, 196, 130, 36)) {
            s.cal = calTmp_;
            s.cal.valid = 1;
            commit();
            backToMain();
        } else if (inRect(e.x, e.y, 170, 196, 130, 36)) {
            startCalibration();
        }
        return;
    }
    gfx().fillCircle(e.x, e.y, 5, rgb(60, 220, 120));
}

// ------------------------------------------------------------------------------------------
// Notifications
// ------------------------------------------------------------------------------------------
namespace {
int wrapLines(TextFace face, const char *text, int maxW, char lines[3][100]) {
    int n = 0;
    char cur[100] = "";
    const char *p = text;
    bool more = false;
    while (*p) {
        char word[100];
        int wl = 0;
        while (*p && *p != ' ' && wl < 90) word[wl++] = *p++;
        word[wl] = 0;
        while (*p == ' ') ++p;
        char trial[200];
        if (cur[0]) snprintf(trial, sizeof(trial), "%s %s", cur, word);
        else snprintf(trial, sizeof(trial), "%s", word);
        if (textWidth(face, trial) <= maxW) { snprintf(cur, sizeof(cur), "%s", trial); continue; }
        if (!cur[0]) { fitText(face, word, maxW); snprintf(cur, sizeof(cur), "%s", word); continue; }
        snprintf(lines[n++], 100, "%s", cur);
        snprintf(cur, sizeof(cur), "%s", word);
        if (n == 3) { more = true; break; }
    }
    if (n < 3 && cur[0]) snprintf(lines[n++], 100, "%s", cur);
    else if (n == 3 && cur[0]) more = true;
    if (more) {
        char tmp[110];
        snprintf(tmp, sizeof(tmp), "%s...", lines[2]);
        fitText(face, tmp, maxW);
        snprintf(lines[2], 100, "%s", tmp);
    }
    return n;
}
}  // namespace

void App::pushNotice(const char *app, const char *text, bool preview) {
    if (screenOff_) return;
    if (screen_ != SCR_MAIN) {
        if (!preview) return;
        backToMain();
    }
    snprintf(notice_.app, sizeof(notice_.app), "%s", app);
    snprintf(notice_.text, sizeof(notice_.text), "%s", text);
    notice_.startMs = hal_millis();
    notice_.durMs = (uint32_t)s.ntfSeconds * 1000U;
    notice_.preview = preview;
    notice_.active = s.ntfPopup || s.ntfTicker;
    lastNoticeDraw_ = 0;
    flashPhase_ = -1;
    dirty_ = true;
    if (!notice_.active && returnToSettings_) { returnToSettings_ = false; openSettings(3); }
}

void App::endNotice() {
    notice_.active = false;
    dirty_ = true;
    flashPhase_ = -1;
    if (returnToSettings_) { returnToSettings_ = false; openSettings(3); }
}

void App::updateNotice() {
    if (notice_.active && hal_millis() - notice_.startMs >= notice_.durMs) endNotice();
}

void App::drawNotice(uint32_t ms) {
    const uint32_t el = ms - notice_.startMs;
    const TextFace face = kNoticeFaces[s.ntfSize];
    Gfx &g = gfx();
    if (s.ntfPopup) {
        char lines[3][100] = {"", "", ""};
        const int n = wrapLines(face, notice_.text, 264, lines);
        const int lh = textHeight(face) + 2;
        const int ah = textHeight(TF_B14) + 4;
        const int cardH = 14 + ah + n * lh + 8;
        const int cx = 16, cardW = 288;
        const int cy = (H - cardH) / 2 - 8;
        bool draw = lastNoticeDraw_ == 0;
        int offs = 0;
        uint16_t border = s.ntfFg;
        if (s.ntfEffect == 1 && el < 320) { offs = (H - cy) * (int)(320 - el) / 320; draw = true; }
        if (s.ntfEffect == 2) {
            const int phase = (int)((el / 250) % 2);
            border = phase ? C_ACC : s.ntfFg;
            if (phase != flashPhase_) { flashPhase_ = phase; draw = true; }
        }
        if (draw) {
            if (offs > 0) { drawBackground(); drawClockAll(); drawDateLine(); }
            const int y = cy + offs;
            g.fillRoundRect(cx, y, cardW, cardH, 12, s.ntfBg);
            g.drawRoundRect(cx, y, cardW, cardH, 12, border);
            g.drawRoundRect(cx + 1, y + 1, cardW - 2, cardH - 2, 11, border);
            const Bg cb = Bg::solid(s.ntfBg);
            drawText(TF_B14, cx + 14, y + 10, notice_.app, mix565(s.ntfBg, s.ntfFg, 190), cb);
            for (int i = 0; i < n; ++i) drawText(face, cx + 14, y + 10 + ah + i * lh, lines[i], s.ntfFg, cb);
        }
    }
    if (s.ntfEffect == 3) {
        const int phase = el < 900 ? (int)((el / 150) % 2) : 0;
        if (phase != flashPhase_) {
            flashPhase_ = phase;
            if (phase) {
                g.fillRect(0, 0, W, 6, s.ntfFg); g.fillRect(0, H - 6, W, 6, s.ntfFg);
                g.fillRect(0, 6, 6, H - 12, s.ntfFg); g.fillRect(W - 6, 6, 6, H - 12, s.ntfFg);
            } else {
                fillBgRows(bg_, 0, 0, W, 6); fillBgRows(bg_, 0, H - 6, W, 6);
                fillBgRows(bg_, 0, 6, 6, H - 12); fillBgRows(bg_, W - 6, 6, 6, H - 12);
            }
        }
    }
    if (s.ntfTicker) {
        char full[160];
        snprintf(full, sizeof(full), "%s: %s", notice_.app, notice_.text);
        const int tw = textWidth(face, full);
        const int bandH = textHeight(face) + 10;
        const int x = tw <= W - 16 ? (W - tw) / 2 : W - (int)((el * 70 / 1000) % (uint32_t)(tw + W));
        drawTextInBand(face, x, 0, H - bandH, W, bandH, full, s.ntfFg, Bg::solid(s.ntfBg));
    }
    lastNoticeDraw_ = ms ? ms : 1;
}
