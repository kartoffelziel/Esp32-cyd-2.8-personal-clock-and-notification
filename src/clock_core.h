#pragma once
#include <stdint.h>

struct LocalTime {
    int year, month, day;   // month 1..12
    int weekday;            // 0 = Sunday
    int hour, minute, second;
    int millisecond;
};

// Pure conversion, verified against Python's datetime in the host tests.
LocalTime localTimeFromUnix(int64_t unixSeconds, int tzOffsetMinutes, int millisecond);

// Four-slot sky colours (sunrise 6:00, day 9:00, sunset 18:00, night 21:00), blended linearly.
void skyForMinute(int minuteOfDay, uint16_t &top, uint16_t &bottom);
uint16_t scaleColor(uint16_t color, int percent);
uint16_t hsv565(int hueDeg, int sat255, int val255);
