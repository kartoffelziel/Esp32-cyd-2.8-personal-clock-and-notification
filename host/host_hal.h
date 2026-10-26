#pragma once
#include <stdint.h>
// Test controls for the simulated board.
extern uint32_t g_ms;
void host_set_time(int64_t unixSeconds);
void host_clear_time();
void host_touch(bool down, int rawX, int rawY);
void host_advance(uint32_t ms);
extern int g_backlight;
extern int g_flipped;
