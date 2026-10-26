#pragma once
#include "../src/gfx.h"
#include <stdint.h>
#include <vector>

// Software framebuffer used by the PC simulator (320x240 RGB565).
class HostGfx : public Gfx {
public:
    HostGfx() : fb_(320 * 240, 0) {}
    int width() const override { return 320; }
    int height() const override { return 240; }
    void fillRect(int x, int y, int w, int h, uint16_t c) override;
    void fillRoundRect(int x, int y, int w, int h, int r, uint16_t c) override;
    void drawRoundRect(int x, int y, int w, int h, int r, uint16_t c) override;
    void fillCircle(int x, int y, int r, uint16_t c) override;
    void drawCircle(int x, int y, int r, uint16_t c) override;
    void drawLine(int x0, int y0, int x1, int y1, uint16_t c) override;
    void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t c) override;
    void blit(int x, int y, int w, int h, const uint16_t *px) override;
    void pixel(int x, int y, uint16_t c) { if (x >= 0 && y >= 0 && x < 320 && y < 240) fb_[y * 320 + x] = c; }
    uint16_t at(int x, int y) const { return fb_[y * 320 + x]; }
    void dump(const char *path) const;   // raw RGB565 little endian
    long blits = 0, fills = 0;
private:
    std::vector<uint16_t> fb_;
};
