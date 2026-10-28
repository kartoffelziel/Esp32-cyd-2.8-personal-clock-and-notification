#pragma once
#include "clock_core.h"
#include "gfx.h"
#include "hal.h"
#include "settings.h"
#include "text.h"
#include <functional>
#include <string>
#include <stdint.h>
#include <vector>

struct TouchEvt {
    enum Kind { NONE, DOWN, MOVE, UP } kind;
    int x, y;
};

class App {
public:
    Settings s;

    void begin();
    void tick();                                    // call every loop iteration
    void handleTouch(const TouchEvt &e);            // calibrated screen coordinates
    // Shows a phone notification (also used by the settings preview).
    void pushNotice(const char *app, const char *text, bool preview = false);

    enum Screen { SCR_MAIN, SCR_SETTINGS, SCR_ADJUST, SCR_KEYBOARD, SCR_CALIB };
    Screen screen() const { return screen_; }
    void openSettings(int tab);
    int tab() const { return tab_; }
    bool screenIsOff() const { return screenOff_; }
    bool noticeActive() const { return notice_.active; }

private:
    // ---- generic row based settings ------------------------------------------------
    enum RowKind { RK_LABEL, RK_SLIDER, RK_TOGGLE, RK_CHOICE, RK_STEPPER, RK_BUTTON, RK_SWATCH, RK_NET };
    struct Row {
        RowKind kind = RK_LABEL;
        char label[56] = "";
        std::function<int()> get;
        std::function<void(int)> set;
        int mn = 0, mx = 1, step = 1;
        const char *const *names = nullptr;
        int nNames = 0;
        const uint16_t *palette = nullptr;
        std::function<void()> onTap;
        std::function<void(char *, int)> fmt;
        bool enabled = true;
        bool connected = false;      // RK_NET: currently connected network
        bool secure = false;
        int rssi = 0;
        int h = 36;
        int y = 0;
    };
    struct Notice {
        bool active = false, preview = false;
        char app[24] = "";
        char text[120] = "";
        uint32_t startMs = 0, durMs = 8000;
    };

    // main screen
    void drawMainFull();
    void updateMain();
    void drawBackground();
    void drawButtons();
    void drawClockAll();
    void drawDateLine();
    struct Geom;
    Geom geom() const;
    void computeBg();
    uint16_t digitColor(uint32_t nowMs) const;
    int digitStrength(int cell, uint32_t nowMs) const;
    void mainTouch(const TouchEvt &e);
    void doPower();
    void wake();
    void setButtonsVisible(bool visible);
    void drawButton(int which, bool pressed);

    // settings
    Row &addRow(RowKind kind, const char *label, int h);
    void addToggle(const char *label, uint8_t *p);
    void addChoice(const char *label, uint8_t *p, const char *const *names, int n);
    void addSlider(const char *label, std::function<int()> get, std::function<void(int)> set, int mn, int mx,
                   std::function<void(char *, int)> fmt);
    void addSwatch(const char *label, const uint16_t *palette, uint16_t *p);
    void addButton(const char *label, std::function<void()> onTap);
    void addLabel(const char *text);
    void buildRows();
    void rowsGeneral();
    void rowsConnect();
    void rowsClock();
    void rowsNotify();
    void drawSettingsFull();
    void drawTabs();
    void drawRows();
    void drawRow(const Row &r, int top);
    void settingsTouch(const TouchEvt &e);
    void activateRow(int index, int x);
    void setSliderFromX(Row &r, int x);
    int rowAt(int y) const;
    int contentHeight() const;
    void commit();
    void pollConnect();

    // adjust screen
    void openAdjust();
    void drawAdjustFull();
    void adjustStep(int code);
    void adjustTouch(const TouchEvt &e);

    // keyboard
    void openKeyboard(const char *title, const char *initial, bool secret,
                      std::function<void(const char *)> done);
    void drawKeyboard();
    void keyboardTouch(const TouchEvt &e);

    // calibration
    void startCalibration();
    void drawCalib();
    void calibRaw(bool valid, bool holding, int rx, int ry);
    void calibTouch(const TouchEvt &e);

    // notification
    void updateNotice();
    void drawNotice(uint32_t nowMs);
    void endNotice();

    // helpers
    bool timeOk() const;
    LocalTime now(uint32_t nowMs) const;
    void backToMain();

    Screen screen_ = SCR_MAIN;
    // main
    Bg bg_ = Bg::solid(0);
    uint16_t skyTop_ = 0, skyBot_ = 0;
    int skyKey_ = -1;
    bool dirty_ = true;
    bool uiVisible_ = true;
    uint32_t lastInteract_ = 0;
    int shown_[4] = {-9, -9, -9, -9};
    int shownStrength_[4] = {-1, -1, -1, -1};
    uint16_t shownColor_ = 0;
    int shownColon_ = -1;
    int shownLayout_ = -1;
    int shownDateKey_ = -1;
    int shownAmPm_ = -1;
    int64_t lastMinuteKey_ = -1;
    uint32_t fadeStart_[4] = {0, 0, 0, 0};
    uint32_t lastEffectDraw_ = 0;
    bool screenOff_ = false;
    bool ignoreUntilUp_ = false;
    // touch tracking
    bool pressed_ = false;
    int downX_ = 0, downY_ = 0, lastX_ = 0, lastY_ = 0;
    uint32_t downMs_ = 0;
    int stable_ = 0;
    int warm_ = 0;
    long warmX_ = 0, warmY_ = 0;
    uint32_t lastRawMs_ = 0;
    int pressedBtn_ = 0;           // 1 = gear, 2 = power while a finger rests on it
    // Recent mapped touch samples (newest last). Readings at the moment the finger lifts are
    // unreliable on a resistive panel, so taps and releases use older, stable samples.
    struct TouchSample { uint32_t ms; int x, y; };
    TouchSample ring_[8];
    int ringCount_ = 0;
    void pushSample(uint32_t ms, int x, int y);
    void medianRecent(int &x, int &y) const;
    void stableBefore(uint32_t beforeMs, int &x, int &y) const;
    // settings
    int tab_ = 0;
    std::vector<Row> rows_;
    int scroll_ = 0, scrollStart_ = 0;
    int downRow_ = -1, activeSlider_ = -1;
    bool dragging_ = false, tabPress_ = false;
    int tabDown_ = -1;
    uint32_t lastScrollDraw_ = 0;
    bool scanning_ = false;
    std::vector<WifiAp> aps_;
    int wifiPrev_ = -1;
    uint32_t lastPoll_ = 0;
    bool ntpRequested_ = false;
    bool confirmReset_ = false;
    // adjust
    int adjustHold_ = -1;
    uint32_t adjustNext_ = 0;
    // keyboard
    char kbTitle_[48] = "";
    char kbText_[66] = "";
    bool kbShift_ = false, kbSymbols_ = false, kbSecret_ = true, kbShow_ = false;
    std::function<void(const char *)> kbDone_;
    // calibration
    int calStep_ = 0;
    int calRaw_[4][2] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
    long calSumX_ = 0, calSumY_ = 0;
    int calN_ = 0;
    int calSkip_ = 0;
    Calibration calTmp_{};
    bool calTest_ = false;
    int calMsg_ = 0;
    // notification
    Notice notice_;
    bool returnToSettings_ = false;
    uint32_t lastNoticeDraw_ = 0;
    int flashPhase_ = -1;
};
