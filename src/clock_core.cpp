#include "clock_core.h"

#include "gfx.h"

static int64_t floorDiv(int64_t v, int64_t d) {
    int64_t q = v / d;
    if ((v % d != 0) && ((v < 0) != (d < 0))) --q;
    return q;
}

LocalTime localTimeFromUnix(int64_t unixSeconds, int tzOffsetMinutes, int millisecond) {
    const int64_t local = unixSeconds + (int64_t)tzOffsetMinutes * 60;
    const int64_t days = floorDiv(local, 86400);
    const int64_t sod = local - days * 86400;
    const int64_t z = days + 719468;   // Howard Hinnant's civil_from_days
    const int64_t era = floorDiv(z, 146097);
    const int64_t doe = z - era * 146097;
    const int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    int64_t year = yoe + era * 400;
    const int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const int64_t mp = (5 * doy + 2) / 153;
    LocalTime t;
    t.day = (int)(doy - (153 * mp + 2) / 5 + 1);
    t.month = (int)(mp < 10 ? mp + 3 : mp - 9);
    if (t.month <= 2) ++year;
    t.year = (int)year;
    t.weekday = (int)(((days % 7) + 7 + 4) % 7);   // 1970-01-01 was a Thursday
    t.hour = (int)(sod / 3600);
    t.minute = (int)((sod % 3600) / 60);
    t.second = (int)(sod % 60);
    t.millisecond = millisecond;
    return t;
}

namespace {
struct Rgb { int r, g, b; };
struct Slot { int minute; Rgb top, bottom; };
// night, sunrise, day, sunset anchors in order; night appears twice to wrap midnight.
const Slot kSlots[] = {
    {360, {40, 50, 120}, {255, 150, 70}},     // sunrise 6:00
    {540, {25, 100, 210}, {110, 185, 235}},   // day     9:00
    {1080, {55, 35, 110}, {255, 110, 55}},    // sunset 18:00
    {1260, {6, 8, 30}, {14, 12, 48}},         // night  21:00
};
int lerp(int a, int b, int num, int den) { return a + (b - a) * num / den; }
uint16_t pack(const Rgb &c) { return rgb((uint8_t)c.r, (uint8_t)c.g, (uint8_t)c.b); }
}  // namespace

void skyForMinute(int minuteOfDay, uint16_t &top, uint16_t &bottom) {
    int m = ((minuteOfDay % 1440) + 1440) % 1440;
    int i = 3;                       // default: between night (21:00) and sunrise (6:00 next day)
    int span = 1440 - 1260 + 360;
    int pos = m >= 1260 ? m - 1260 : m + (1440 - 1260);
    if (m >= 360 && m < 540) { i = 0; span = 540 - 360; pos = m - 360; }
    else if (m >= 540 && m < 1080) { i = 1; span = 1080 - 540; pos = m - 540; }
    else if (m >= 1080 && m < 1260) { i = 2; span = 1260 - 1080; pos = m - 1080; }
    const Slot &a = kSlots[i];
    const Slot &b = kSlots[(i + 1) % 4];
    const Rgb t = {lerp(a.top.r, b.top.r, pos, span), lerp(a.top.g, b.top.g, pos, span), lerp(a.top.b, b.top.b, pos, span)};
    const Rgb u = {lerp(a.bottom.r, b.bottom.r, pos, span), lerp(a.bottom.g, b.bottom.g, pos, span), lerp(a.bottom.b, b.bottom.b, pos, span)};
    top = pack(t);
    bottom = pack(u);
}

uint16_t scaleColor(uint16_t c, int percent) {
    if (percent > 100) percent = 100;
    if (percent < 0) percent = 0;
    return rgb((uint8_t)(r8(c) * percent / 100), (uint8_t)(g8(c) * percent / 100), (uint8_t)(b8(c) * percent / 100));
}

uint16_t hsv565(int hue, int sat, int val) {
    hue = ((hue % 360) + 360) % 360;
    const int region = hue / 60, rem = (hue % 60) * 255 / 60;
    const int p = val * (255 - sat) / 255;
    const int q = val * (255 - sat * rem / 255) / 255;
    const int t = val * (255 - sat * (255 - rem) / 255) / 255;
    int r = 0, g = 0, b = 0;
    switch (region) {
        case 0: r = val; g = t; b = p; break;
        case 1: r = q; g = val; b = p; break;
        case 2: r = p; g = val; b = t; break;
        case 3: r = p; g = q; b = val; break;
        case 4: r = t; g = p; b = val; break;
        default: r = val; g = p; b = q; break;
    }
    return rgb((uint8_t)r, (uint8_t)g, (uint8_t)b);
}
