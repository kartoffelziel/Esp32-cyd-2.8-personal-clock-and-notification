#include "host_gfx.h"
#include "host_hal.h"
#include "../src/ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

static HostGfx g_gfx;
Gfx &gfx() { return g_gfx; }
static int g_fail = 0;
#define CHECK(c) do { if (!(c)) { printf("CHECK FAILED line %d: %s\n", __LINE__, #c); ++g_fail; } } while (0)

static void shot(const char *name) { char p[96]; snprintf(p, sizeof p, "/tmp/ui_%s.raw", name); g_gfx.dump(p); }
static void run(App &a, int ms) { for (int i = 0; i < ms / 20; ++i) { host_advance(20); a.tick(); } }
static void tap(App &a, int x, int y) {
    a.handleTouch({TouchEvt::DOWN, x, y});
    host_advance(60);
    a.handleTouch({TouchEvt::UP, x, y});
    host_advance(60);
}

static int tabCenter(int i) {
    static const char *n[4] = {"Chung", "Kết nối", "Đồng hồ", "Thông báo"};
    int w[4], sum = 0;
    for (int k = 0; k < 4; ++k) { w[k] = textWidth(TF_R14, n[k]) + 8; sum += w[k]; }
    const int extra = (280 - sum) / 4;
    int x = 0;
    for (int k = 0; k < 4; ++k) { const int ww = w[k] + extra; if (k == i) return x + ww / 2; x += ww; }
    return 0;
}

// raw touch the way a swapped + inverted panel would report a screen point
static void rawFor(int x, int y, int &rx, int &ry) { rx = 3900 - y * 14; ry = 250 + x * 11; }
static void hold(App &a, int x, int y, int ticks = 14) {
    int rx, ry; rawFor(x, y, rx, ry);
    host_touch(true, rx, ry);
    for (int i = 0; i < ticks; ++i) { host_advance(20); a.tick(); }
    host_touch(false, 0, 0);
    for (int i = 0; i < 6; ++i) { host_advance(20); a.tick(); }
}

int main() {
    host_set_time(1761305100);                       // 2025-10-24 18:25:00 GMT+7
    App a;
    settingsDefaults(a.s);
    a.s.cal.valid = 1; a.s.cal.swap = 0; a.s.cal.a0 = 300; a.s.cal.a1 = 3800; a.s.cal.b0 = 300; a.s.cal.b1 = 3800;
    settingsSave(a.s);
    a.begin();
    run(a, 200);
    shot("01_main");
    CHECK(a.screen() == App::SCR_MAIN);

    // tap an empty area hides the buttons, tap again shows them
    tap(a, 160, 140);
    run(a, 100);
    shot("02_main_hidden");
    tap(a, 160, 140);
    run(a, 100);

    // gear -> settings, general tab
    tap(a, 24, 24);
    CHECK(a.screen() == App::SCR_SETTINGS);
    shot("03_general");
    // drag the backlight slider to ~50 %
    a.handleTouch({TouchEvt::DOWN, 30, 32 + 40});
    a.handleTouch({TouchEvt::MOVE, 160, 32 + 40});
    a.handleTouch({TouchEvt::UP, 160, 32 + 40});
    printf("backlight now %d (hal %d)\n", a.s.backlight, g_backlight);
    CHECK(a.s.backlight > 100 && a.s.backlight < 160 && g_backlight == a.s.backlight);
    // scroll down by dragging
    a.handleTouch({TouchEvt::DOWN, 100, 200});
    host_advance(50); a.handleTouch({TouchEvt::MOVE, 100, 120});
    host_advance(50); a.handleTouch({TouchEvt::MOVE, 100, 60});
    a.handleTouch({TouchEvt::UP, 100, 60});
    shot("04_general_scrolled");

    // connect tab (auto scan)
    tap(a, tabCenter(1), 16);
    CHECK(a.tab() == 1);
    run(a, 2000);
    shot("05_connect");
    // clock tab
    tap(a, tabCenter(2), 16);
    CHECK(a.tab() == 2);
    shot("06_clock");
    // choose the next digit font by tapping the first row, then set size
    const uint8_t font0 = a.s.clockFont;
    tap(a, 160, 32 + 20);
    CHECK(a.s.clockFont == (uint8_t)((font0 + 1) % (kDigitFontCount + kLedStyles)));
    // notify tab
    tap(a, tabCenter(3), 16);
    CHECK(a.tab() == 3);
    shot("07_notify");
    // preview notification
    tap(a, 160, 32 + 40 * 2 + 40 + 64 + 64 + 40 + 56 + 23);    // "Xem trước" button row
    run(a, 400);
    CHECK(a.noticeActive());
    shot("08_notice_popup");
    run(a, 3000);
    shot("09_notice_later");
    run(a, 6000);
    CHECK(!a.noticeActive());
    CHECK(a.screen() == App::SCR_SETTINGS && a.tab() == 3);
    // back to main
    tap(a, 290, 16);
    CHECK(a.screen() == App::SCR_MAIN);
    run(a, 200);
    shot("10_main_after");


    // ---- clock variants ------------------------------------------------------------------
    {
        App b; settingsDefaults(b.s); b.s.cal = a.s.cal; b.s.cal.valid = 1; b.s.autoHideSec = 0;
        settingsSave(b.s); b.begin(); run(b, 120);
        b.s.clockFont = 7; b.s.clockSize = 130; b.s.hour24 = 0; b.s.leadingZero = 0; b.s.clockColor = rgb(60, 220, 230); b.s.dateSize = 2;
        host_set_time(1761305100 + 3600 * 12 - 3600 * 2);   // 03:25 GMT+7... -> 16:25? keep an afternoon time
        run(b, 120); shot("11_led_12h");
        b.s.clockFont = 5; b.s.clockSize = 90; b.s.hour24 = 1; b.s.clockColor = rgb(255, 214, 10); b.s.autoSky = 0; b.s.bgColor = rgb(8, 16, 56); b.s.dateSize = 1;
        run(b, 120); shot("12_thin_navy");
        host_set_time(1761305100 + 3600 * 3 + 600);          // 21:35 local -> night sky
        b.s.autoSky = 1; b.s.clockFont = 1; b.s.clockSize = 100; b.s.clockEffect = 2; b.s.clockColor = rgb(255, 255, 255);
        run(b, 600); shot("13_mono_rainbow_night");
        host_clear_time(); b.s.clockEffect = 0; run(b, 200); shot("14_unsynced");
        host_set_time(1761305100);
    }
    // ---- keyboard + wifi password ---------------------------------------------------------
    {
        App b; settingsDefaults(b.s); b.s.cal = a.s.cal; b.s.cal.valid = 1; settingsSave(b.s); b.begin(); run(b, 100);
        b.openSettings(1); run(b, 2000);
        tap(b, 100, 32 + 30 + 46 + 19 + 0);   // first network row ("Nha Cua Vang")
        CHECK(b.screen() == App::SCR_KEYBOARD);
        for (char c : std::string("Hello123")) {
            // find the key by simulating a tap at the known layout positions
            int x = 0, y = 0;
            const char *r1 = "1234567890", *r2 = "qwertyuiop", *r3 = "asdfghjkl", *r4 = "zxcvbnm";
            const char *p;
            if (c >= 'A' && c <= 'Z') { tap(b, 16, 166 + 17); c = (char)(c + 32); }   // shift
            if ((p = strchr(r1, c))) { x = (int)(p - r1) * 32 + 16; y = 58 + 17; }
            else if ((p = strchr(r2, c))) { x = (int)(p - r2) * 32 + 16; y = 94 + 17; }
            else if ((p = strchr(r3, c))) { x = 16 + (int)(p - r3) * 32 + 16; y = 130 + 17; }
            else if ((p = strchr(r4, c))) { x = 48 + (int)(p - r4) * 32 + 16; y = 166 + 17; }
            tap(b, x, y);
        }
        shot("15_keyboard");
        tap(b, 292, 202 + 17);                 // OK
        CHECK(b.screen() == App::SCR_SETTINGS);
        printf("saved ssid=[%s] pass=[%s]\n", b.s.ssid, b.s.pass);
        CHECK(strcmp(b.s.pass, "Hello123") == 0);
        run(b, 300); shot("16_connect_connecting");
    }
    // ---- adjust screen ----------------------------------------------------------------------
    {
        App b; settingsDefaults(b.s); b.s.cal = a.s.cal; b.s.cal.valid = 1; settingsSave(b.s); b.begin(); run(b, 100);
        b.openSettings(2);
        tap(b, 160, 32 + 40 + 56 + 23);        // "Chỉnh vị trí" button
        CHECK(b.screen() == App::SCR_ADJUST);
        for (int i = 0; i < 6; ++i) tap(b, 260, 166);       // up x6
        for (int i = 0; i < 5; ++i) tap(b, 302, 208);       // right x5
        printf("clock offset X=%d Y=%d\n", b.s.clockX, b.s.clockY);
        CHECK(b.s.clockX == 10 && b.s.clockY == -12);
        shot("17_adjust");
        tap(b, 280, 18);
        CHECK(b.screen() == App::SCR_SETTINGS);
    }
    // ---- calibration through the raw path -------------------------------------------------
    {
        App b; settingsDefaults(b.s); b.s.cal.valid = 0; settingsSave(b.s); b.begin(); run(b, 60);
        CHECK(b.screen() == App::SCR_CALIB);
        shot("18_calib_step1");
        hold(b, 20, 20); hold(b, 299, 20); hold(b, 20, 219); hold(b, 299, 219);
        shot("19_calib_test");
        // touch the middle: the green dot must land near (160,120)
        int rx, ry; rawFor(160, 120, rx, ry);
        host_touch(true, rx, ry);
        for (int i = 0; i < 6; ++i) { host_advance(20); b.tick(); }
        int hits = 0;
        for (int y = 112; y <= 128; ++y) for (int x = 152; x <= 168; ++x) if (g_gfx.at(x, y) == rgb(60, 220, 120)) ++hits;
        printf("dot pixels near centre: %d\n", hits);
        CHECK(hits > 40);
        host_touch(false, 0, 0);
        for (int i = 0; i < 6; ++i) { host_advance(20); b.tick(); }
        hold(b, 85, 214);                       // "Lưu"
        CHECK(b.screen() == App::SCR_MAIN);
        CHECK(b.s.cal.valid == 1 && b.s.cal.swap == 1);
        printf("cal swap=%d a=[%d,%d] b=[%d,%d]\n", b.s.cal.swap, b.s.cal.a0, b.s.cal.a1, b.s.cal.b0, b.s.cal.b1);
    }
    // ---- power: screen off and touch wake -------------------------------------------------
    {
        App b; settingsDefaults(b.s); b.s.cal = a.s.cal; b.s.cal.valid = 1; settingsSave(b.s); b.begin(); run(b, 100);
        tap(b, 296, 24);                         // power icon
        CHECK(b.screenIsOff() && g_backlight == 0);
        tap(b, 100, 100);                        // wake
        CHECK(!b.screenIsOff() && g_backlight == b.s.backlight);
        host_advance(100);
        tap(b, 100, 100);                        // the wake touch must not have toggled anything else
        CHECK(b.screen() == App::SCR_MAIN);
    }
    printf("fails=%d blits=%ld\n", g_fail, g_gfx.blits);
    return g_fail != 0;
}
