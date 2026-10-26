#pragma once
#include <stdint.h>

constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}
inline uint8_t r8(uint16_t c) { return (uint8_t)(((c >> 11) & 31) * 255 / 31); }
inline uint8_t g8(uint16_t c) { return (uint8_t)(((c >> 5) & 63) * 255 / 63); }
inline uint8_t b8(uint16_t c) { return (uint8_t)((c & 31) * 255 / 31); }

// Linear blend, t = 0..255 (0 = a, 255 = b).
inline uint16_t mix565(uint16_t a, uint16_t b, int t) {
    const int ra = (a >> 11) & 31, ga = (a >> 5) & 63, ba = a & 31;
    const int rb = (b >> 11) & 31, gb = (b >> 5) & 63, bb = b & 31;
    const int r = ra + (rb - ra) * t / 255;
    const int g = ga + (gb - ga) * t / 255;
    const int bl = ba + (bb - ba) * t / 255;
    return (uint16_t)((r << 11) | (g << 5) | bl);
}

// Minimal drawing surface. Colours are native RGB565 (not byte swapped).
struct Gfx {
    virtual ~Gfx() {}
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual void fillRect(int x, int y, int w, int h, uint16_t c) = 0;
    virtual void fillRoundRect(int x, int y, int w, int h, int r, uint16_t c) = 0;
    virtual void drawRoundRect(int x, int y, int w, int h, int r, uint16_t c) = 0;
    virtual void fillCircle(int x, int y, int r, uint16_t c) = 0;
    virtual void drawCircle(int x, int y, int r, uint16_t c) = 0;
    virtual void drawLine(int x0, int y0, int x1, int y1, uint16_t c) = 0;
    virtual void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t c) = 0;
    virtual void blit(int x, int y, int w, int h, const uint16_t *pixels) = 0;
};
Gfx &gfx();   // provided by the platform (board or simulator)
