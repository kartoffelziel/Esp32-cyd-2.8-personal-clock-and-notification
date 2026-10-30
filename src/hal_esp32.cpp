// Everything hardware specific for the ESP32-2432S028 ("Cheap Yellow Display").
#include <Arduino.h>
#include <Preferences.h>
#include <SPI.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include <driver/gpio.h>
#include <sys/time.h>
#include <time.h>

#include "hal.h"

void gfx_init(bool flipped);
void gfx_set_flipped(bool flipped);

namespace {
constexpr int kBacklightPin = 21;
constexpr int kTouchClk = 25, kTouchMosi = 32, kTouchMiso = 39, kTouchCs = 33, kTouchIrq = 36;
constexpr int kBootPin = 0;

SPIClass touchSpi(VSPI);
Preferences prefs;

int readXpt(uint8_t command) {
    touchSpi.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(kTouchCs, LOW);
    touchSpi.transfer(command);
    const uint8_t hi = touchSpi.transfer(0);
    const uint8_t lo = touchSpi.transfer(0);
    digitalWrite(kTouchCs, HIGH);
    touchSpi.endTransaction();
    return (((hi << 8) | lo) >> 3) & 0x0FFF;
}

int median3(int a, int b, int c) {
    const int lo = a < b ? a : b, hi = a < b ? b : a;
    return c < lo ? lo : (c > hi ? hi : c);
}

bool g_irqUsable = false;       // /PENIRQ idles high at boot, so it can be trusted as a pen-down signal
enum { REQ_NONE, REQ_CONNECT } g_wifiReq = REQ_NONE;
uint32_t g_wifiStart = 0;
}  // namespace

void hal_init() {
    const int leds[] = {4, 16, 17};            // RGB LED is active low: switch it off
    for (int pin : leds) { pinMode(pin, OUTPUT); digitalWrite(pin, HIGH); }
    pinMode(kTouchCs, OUTPUT);
    digitalWrite(kTouchCs, HIGH);
    pinMode(kTouchIrq, INPUT);
    pinMode(kBootPin, INPUT_PULLUP);
    touchSpi.begin(kTouchClk, kTouchMiso, kTouchMosi, kTouchCs);
    // Idle level check: if the pin does not sit high while nobody touches, ignore it (not wired / stuck).
    delay(30);
    g_irqUsable = true;
    for (int i = 0; i < 20; ++i) { if (digitalRead(kTouchIrq) != HIGH) g_irqUsable = false; delay(4); }
    ledcSetup(0, 5000, 8);
    ledcAttachPin(kBacklightPin, 0);
    ledcWrite(0, 0);                            // dark until the first frame is drawn
}

void hal_init_display(bool flipped) { gfx_init(flipped); }

// ---- time ---------------------------------------------------------------------------------------
uint32_t hal_millis() { return millis(); }

int64_t hal_unix_ms() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void hal_set_unix(int64_t seconds) {
    struct timeval tv = {(time_t)seconds, 0};
    settimeofday(&tv, nullptr);
}

bool hal_time_valid() { return time(nullptr) > 1700000000; }   // later than Nov 2023

// ---- display ------------------------------------------------------------------------------------------
void hal_backlight(uint8_t level) { ledcWrite(0, level); }
void hal_set_flipped(bool flipped) { gfx_set_flipped(flipped); }

// ---- touch -----------------------------------------------------------------------------------------------
namespace {
// Mean of the middle values of n samples (drops the min and the max): stable even on a light touch.
int trimmedMean(int *v, int n) {
    int lo = v[0], hi = v[0], sum = 0;
    for (int i = 0; i < n; ++i) { if (v[i] < lo) lo = v[i]; if (v[i] > hi) hi = v[i]; sum += v[i]; }
    return (sum - lo - hi) / (n - 2);
}
}  // namespace

bool hal_touch_read(int &rawX, int &rawY) {
    // /PENIRQ is pulled low by the controller only while the panel is really touched. The pressure
    // formula alone can read "pressed" on some boards even with no finger, which would hold a press
    // open forever (no release -> no tap).
    if (g_irqUsable && digitalRead(kTouchIrq) != LOW) return false;
    const int z1 = readXpt(0xB0);
    const int z2 = readXpt(0xC0);
    if (z1 + 4095 - z2 < 400) return false;      // not pressed hard enough
    readXpt(0xD0);                               // first conversion after idle is noisy
    int xs[6], ys[6];
    for (int i = 0; i < 6; ++i) { xs[i] = readXpt(0xD0); ys[i] = readXpt(0x90); }
    rawX = trimmedMean(xs, 6);
    rawY = trimmedMean(ys, 6);
    return true;
}

bool hal_boot_pressed() { return digitalRead(kBootPin) == LOW; }

// ---- storage -------------------------------------------------------------------------------------------------
size_t hal_store_read(const char *key, void *buf, size_t maxLen) {
    if (!prefs.begin("cydclock", true)) return 0;
    const size_t len = prefs.getBytesLength(key);
    size_t got = 0;
    if (len > 0 && len <= maxLen) got = prefs.getBytes(key, buf, len);
    prefs.end();
    return got;
}

void hal_store_write(const char *key, const void *buf, size_t len) {
    if (!prefs.begin("cydclock", false)) return;
    prefs.putBytes(key, buf, len);
    prefs.end();
}

// ---- Wi-Fi ---------------------------------------------------------------------------------------------------------
void hal_wifi_scan_start() {
    if (WiFi.getMode() == WIFI_OFF) WiFi.mode(WIFI_STA);
    WiFi.scanDelete();
    WiFi.scanNetworks(true);                     // asynchronous
}

int hal_wifi_scan_state() {
    const int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_RUNNING) return -1;
    return n < 0 ? 0 : n;
}

bool hal_wifi_scan_get(int index, WifiAp &out) {
    if (index < 0 || index >= WiFi.scanComplete()) return false;
    memset(&out, 0, sizeof(out));
    const String ssid = WiFi.SSID(index);
    strncpy(out.ssid, ssid.c_str(), sizeof(out.ssid) - 1);
    out.rssi = (int8_t)WiFi.RSSI(index);
    out.secure = WiFi.encryptionType(index) != WIFI_AUTH_OPEN;
    return true;
}

void hal_wifi_connect(const char *ssid, const char *pass) {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.begin(ssid, (pass && pass[0]) ? pass : nullptr);
    g_wifiReq = REQ_CONNECT;
    g_wifiStart = millis();
}

void hal_wifi_disconnect() {
    g_wifiReq = REQ_NONE;
    WiFi.disconnect(true);
}

int hal_wifi_status() {
    if (WiFi.status() == WL_CONNECTED) return WIFI_CONNECTED;
    if (g_wifiReq == REQ_CONNECT) return (millis() - g_wifiStart > 20000) ? WIFI_FAILED : WIFI_CONNECTING;
    return WIFI_IDLE;
}

void hal_wifi_ssid(char *out, size_t n) {
    const String ssid = WiFi.SSID();
    strncpy(out, ssid.c_str(), n - 1);
    out[n - 1] = 0;
}

void hal_ntp_request() { configTime(0, 0, "pool.ntp.org", "time.google.com", "time.cloudflare.com"); }

// ---- power ------------------------------------------------------------------------------------------------------------
void hal_sleep_until_touch() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    const uint32_t t0 = millis();
    while (digitalRead(kTouchIrq) == LOW && millis() - t0 < 1500) delay(10);   // finger still down
    esp_sleep_enable_ext0_wakeup((gpio_num_t)kTouchIrq, 0);                     // XPT2046 /PENIRQ is active low
    gpio_wakeup_enable((gpio_num_t)kBootPin, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();
    delay(30);
    esp_light_sleep_start();                                                    // the clock keeps counting
}
