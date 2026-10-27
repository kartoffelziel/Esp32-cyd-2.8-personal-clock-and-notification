#include "icons.h"

#include <math.h>

namespace {

constexpr float kPi = 3.14159265f;

float clamp01(float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }
float coverage(float sd) { return clamp01(0.5f - sd); }   // 1 px anti-aliased edge

float sdBox(float px, float py, float hx, float hy, float r) {
    const float qx = fabsf(px) - hx + r, qy = fabsf(py) - hy + r;
    const float mx = qx > 0 ? qx : 0, my = qy > 0 ? qy : 0;
    const float inner = qx > qy ? qx : qy;
    return sqrtf(mx * mx + my * my) + (inner < 0 ? inner : 0) - r;
}

float sdSegment(float px, float py, float ax, float ay, float bx, float by) {
    const float pax = px - ax, pay = py - ay, bax = bx - ax, bay = by - ay;
    const float h = clamp01((pax * bax + pay * bay) / (bax * bax + bay * bay));
    const float dx = pax - bax * h, dy = pay - bay * h;
    return sqrtf(dx * dx + dy * dy);
}

// Signed distance of the gear icon (positive outside). Units are px at size 44, scaled by k.
float sdGear(float x, float y, float k) {
    float d = sqrtf(x * x + y * y);
    float sd = d - 7.4f * k;                        // body
    for (int i = 0; i < 8; ++i) {                   // eight rounded teeth
        const float a = i * kPi / 4.f;
        const float c = cosf(a), s = sinf(a);
        const float tx = x * c + y * s, ty = -x * s + y * c;   // into the tooth's frame (radial = +x)
        const float t = sdBox(tx - 8.6f * k, ty, 2.9f * k, 2.2f * k, 0.9f * k);
        if (t < sd) sd = t;
    }
    const float hole = 3.1f * k - d;                // punch the centre hole
    return sd > hole ? sd : hole;
}

// Power symbol: a ring open at the top with a bar through the gap.
float sdPower(float x, float y, float k) {
    const float R = 9.6f * k, th = 1.55f * k;       // ring radius and half thickness
    const float gap = 38.f * kPi / 180.f;           // half opening angle
    const float ang = atan2f(x, -y);                // 0 = straight up, clockwise
    float sd;
    if (fabsf(ang) >= gap) {
        sd = fabsf(sqrtf(x * x + y * y) - R) - th;
    } else {                                        // round end caps at the two arc ends
        const float ex = R * sinf(gap), ey = -R * cosf(gap);
        const float d1 = sqrtf((x - ex) * (x - ex) + (y - ey) * (y - ey));
        const float d2 = sqrtf((x + ex) * (x + ex) + (y - ey) * (y - ey));
        sd = (d1 < d2 ? d1 : d2) - th;
    }
    const float bar = sdSegment(x, y, 0, -R - 1.2f * k, 0, -1.2f * k) - th;
    return sd < bar ? sd : bar;
}

}  // namespace

void drawIconButton(IconKind kind, int x, int y, int size, const Bg &bg, bool pressed) {
    static uint16_t buf[56 * 56];
    if (size > 56) size = 56;
    const float k = size / 44.f;
    const float half = size / 2.f;
    for (int py = 0; py < size; ++py) {
        const uint16_t base = bg.at(y + py);
        const uint16_t fill = pressed ? mix565(base, rgb(255, 255, 255), 70) : mix565(base, rgb(0, 0, 0), 120);
        const uint16_t edge = mix565(base, rgb(255, 255, 255), pressed ? 150 : 80);
        for (int px = 0; px < size; ++px) {
            const float fx = px + 0.5f - half, fy = py + 0.5f - half;
            // rounded square, slightly inset so the anti-aliased edge stays inside the buffer
            const float sd = sdBox(fx, fy, half - 0.6f, half - 0.6f, 13.f * k);
            uint16_t c = base;
            const float inside = coverage(sd);
            if (inside > 0) {
                const float ring = coverage(fabsf(sd + 0.7f) - 0.7f);         // thin border
                c = mix565(c, fill, (int)(inside * 255));
                if (ring > 0) c = mix565(c, edge, (int)(ring * 255));
            }
            const float isd = kind == ICON_GEAR ? sdGear(fx, fy, k) : sdPower(fx, fy, k);
            const float ic = coverage(isd);
            if (ic > 0) c = mix565(c, rgb(255, 255, 255), (int)(ic * 255));
            buf[py * size + px] = c;
        }
    }
    gfx().blit(x, y, size, size, buf);
}
