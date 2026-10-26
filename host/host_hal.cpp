#include "host_hal.h"
#include "../src/hal.h"
#include <map>
#include <string>
#include <string.h>
#include <vector>

uint32_t g_ms = 1000;
int g_backlight = 255, g_flipped = 0;
static int64_t g_baseUnixMs = 0;
static uint32_t g_baseAt = 0;
static bool g_valid = false;
static bool g_down = false;
static int g_rx = 0, g_ry = 0;
static std::map<std::string, std::vector<uint8_t>> g_store;
static uint32_t g_scanStart = 0;
static bool g_scanning = false;
static int g_wifiState = WIFI_IDLE;
static uint32_t g_wifiAt = 0;
static char g_ssid[33] = "";

void host_set_time(int64_t s) { g_baseUnixMs = s * 1000; g_baseAt = g_ms; g_valid = true; }
void host_clear_time() { g_valid = false; }
void host_touch(bool d, int x, int y) { g_down = d; g_rx = x; g_ry = y; }
void host_advance(uint32_t ms) { g_ms += ms; }

uint32_t hal_millis() { return g_ms; }
int64_t hal_unix_ms() { return g_baseUnixMs + (int64_t)(g_ms - g_baseAt); }
void hal_set_unix(int64_t s) { host_set_time(s); }
bool hal_time_valid() { return g_valid; }
void hal_backlight(uint8_t l) { g_backlight = l; }
void hal_set_flipped(bool f) { g_flipped = f; }
bool hal_touch_read(int &x, int &y) { x = g_rx; y = g_ry; return g_down; }

size_t hal_store_read(const char *k, void *buf, size_t max) {
    auto it = g_store.find(k);
    if (it == g_store.end()) return 0;
    const size_t n = it->second.size() < max ? it->second.size() : max;
    memcpy(buf, it->second.data(), n);
    return n;
}
void hal_store_write(const char *k, const void *buf, size_t len) {
    g_store[k] = std::vector<uint8_t>((const uint8_t *)buf, (const uint8_t *)buf + len);
}

static const struct { const char *ssid; int rssi; bool secure; } kFake[] = {
    {"Nha Cua Vang", -48, true}, {"TP-Link_5G_7A2C", -61, true}, {"Cafe Free WiFi", -70, false},
    {"Quán Cà Phê Trà Sữa", -74, true}, {"FPT-Telecom-92", -83, true}, {"Guest", -88, false}};
void hal_wifi_scan_start() { g_scanning = true; g_scanStart = g_ms; }
int hal_wifi_scan_state() {
    if (!g_scanning) return 0;
    if (g_ms - g_scanStart < 1500) return -1;
    g_scanning = false;
    return 6;
}
bool hal_wifi_scan_get(int i, WifiAp &o) {
    if (i < 0 || i >= 6) return false;
    memset(&o, 0, sizeof(o));
    strncpy(o.ssid, kFake[i].ssid, 32);
    o.rssi = (int8_t)kFake[i].rssi;
    o.secure = kFake[i].secure;
    return true;
}
void hal_wifi_connect(const char *ssid, const char *) {
    strncpy(g_ssid, ssid, 32);
    g_wifiState = WIFI_CONNECTING;
    g_wifiAt = g_ms;
}
void hal_wifi_disconnect() { g_wifiState = WIFI_IDLE; g_ssid[0] = 0; }
int hal_wifi_status() {
    if (g_wifiState == WIFI_CONNECTING && g_ms - g_wifiAt > 2000) g_wifiState = WIFI_CONNECTED;
    return g_wifiState;
}
void hal_wifi_ssid(char *out, size_t n) { strncpy(out, g_ssid, n - 1); out[n - 1] = 0; }
void hal_ntp_request() {}
void hal_sleep_until_touch() {}
