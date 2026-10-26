#pragma once
// Everything the UI needs from the hardware. Implemented by hal_esp32.cpp on the
// board and by host/host_hal.cpp for the PC simulator used to test the UI.
#include <stddef.h>
#include <stdint.h>

// ---- time ----------------------------------------------------------------
uint32_t hal_millis();
int64_t hal_unix_ms();                 // wall clock, UTC milliseconds
void hal_set_unix(int64_t seconds);    // used by manual / BLE time sync
bool hal_time_valid();                 // false until some source set the clock

// ---- display ---------------------------------------------------------------
void hal_backlight(uint8_t level);     // 0..255
void hal_set_flipped(bool flipped);    // rotate the picture 180 degrees

// ---- touch (12-bit XPT2046 readings, unrotated) ------------------------------
bool hal_touch_read(int &rawX, int &rawY);

// ---- persistent storage -------------------------------------------------------
size_t hal_store_read(const char *key, void *buf, size_t maxLen);   // bytes read, 0 = missing
void hal_store_write(const char *key, const void *buf, size_t len);

// ---- Wi-Fi ----------------------------------------------------------------------
struct WifiAp {
    char ssid[33];
    int8_t rssi;
    bool secure;
};
enum { WIFI_IDLE = 0, WIFI_CONNECTING, WIFI_CONNECTED, WIFI_FAILED };
void hal_wifi_scan_start();
int hal_wifi_scan_state();             // -1 while scanning, otherwise the number of results
bool hal_wifi_scan_get(int index, WifiAp &out);
void hal_wifi_connect(const char *ssid, const char *pass);
void hal_wifi_disconnect();
int hal_wifi_status();
void hal_wifi_ssid(char *out, size_t n);
void hal_ntp_request();                // (re)start SNTP

// ---- power ------------------------------------------------------------------------
// Light sleep with radios off; returns when the screen is touched or BOOT is pressed.
void hal_sleep_until_touch();
