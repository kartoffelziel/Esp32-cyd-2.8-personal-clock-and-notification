#include "host_gfx.h"
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>

void HostGfx::fillRect(int x, int y, int w, int h, uint16_t c) {
    ++fills;
    for (int yy = y; yy < y + h; ++yy)
        for (int xx = x; xx < x + w; ++xx) pixel(xx, yy, c);
}
void HostGfx::blit(int x, int y, int w, int h, const uint16_t *px) {
    ++blits;
    for (int yy = 0; yy < h; ++yy)
        for (int xx = 0; xx < w; ++xx) pixel(x + xx, y + yy, px[yy * w + xx]);
}
void HostGfx::fillCircle(int cx, int cy, int r, uint16_t c) {
    for (int y = -r; y <= r; ++y)
        for (int x = -r; x <= r; ++x)
            if (x * x + y * y <= r * r + r / 2) pixel(cx + x, cy + y, c);
}
void HostGfx::drawCircle(int cx, int cy, int r, uint16_t c) {
    for (int y = -r - 1; y <= r + 1; ++y)
        for (int x = -r - 1; x <= r + 1; ++x) {
            const int d = x * x + y * y;
            if (d <= r * r + r && d >= (r - 1) * (r - 1) - (r - 1) / 2) pixel(cx + x, cy + y, c);
        }
}
void HostGfx::fillRoundRect(int x, int y, int w, int h, int r, uint16_t c) {
    if (r * 2 > w) r = w / 2;
    if (r * 2 > h) r = h / 2;
    fillRect(x + r, y, w - 2 * r, h, c);
    fillRect(x, y + r, r, h - 2 * r, c);
    fillRect(x + w - r, y + r, r, h - 2 * r, c);
    for (int yy = 0; yy < r; ++yy)
        for (int xx = 0; xx < r; ++xx) {
            const int dx = r - 1 - xx, dy = r - 1 - yy;
            if (dx * dx + dy * dy <= r * r - r / 2) {
                pixel(x + xx, y + yy, c);
                pixel(x + w - 1 - xx, y + yy, c);
                pixel(x + xx, y + h - 1 - yy, c);
                pixel(x + w - 1 - xx, y + h - 1 - yy, c);
            }
        }
}
void HostGfx::drawRoundRect(int x, int y, int w, int h, int r, uint16_t c) {
    if (r * 2 > w) r = w / 2;
    if (r * 2 > h) r = h / 2;
    fillRect(x + r, y, w - 2 * r, 1, c);
    fillRect(x + r, y + h - 1, w - 2 * r, 1, c);
    fillRect(x, y + r, 1, h - 2 * r, c);
    fillRect(x + w - 1, y + r, 1, h - 2 * r, c);
    for (int yy = 0; yy < r; ++yy)
        for (int xx = 0; xx < r; ++xx) {
            const int dx = r - 1 - xx, dy = r - 1 - yy;
            const int d = dx * dx + dy * dy;
            if (d <= r * r - r / 2 && d >= (r - 1) * (r - 1) - (r - 1)) {
                pixel(x + xx, y + yy, c);
                pixel(x + w - 1 - xx, y + yy, c);
                pixel(x + xx, y + h - 1 - yy, c);
                pixel(x + w - 1 - xx, y + h - 1 - yy, c);
            }
        }
}
void HostGfx::drawLine(int x0, int y0, int x1, int y1, uint16_t c) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        pixel(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        const int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}
void HostGfx::fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t c) {
    const int minx = std::min(x0, std::min(x1, x2)), maxx = std::max(x0, std::max(x1, x2));
    const int miny = std::min(y0, std::min(y1, y2)), maxy = std::max(y0, std::max(y1, y2));
    auto edge = [](int ax, int ay, int bx, int by, int px, int py) { return (bx - ax) * (py - ay) - (by - ay) * (px - ax); };
    for (int y = miny; y <= maxy; ++y)
        for (int x = minx; x <= maxx; ++x) {
            const int w0 = edge(x1, y1, x2, y2, x, y), w1 = edge(x2, y2, x0, y0, x, y), w2 = edge(x0, y0, x1, y1, x, y);
            if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0)) pixel(x, y, c);
        }
}
void HostGfx::dump(const char *path) const {
    FILE *f = fopen(path, "wb");
    fwrite(fb_.data(), 2, fb_.size(), f);
    fclose(f);
}
