// Board implementation of the drawing surface (TFT_eSPI on the ILI9341 of the CYD).
#include <Arduino.h>
#include <TFT_eSPI.h>

#include "gfx.h"

TFT_eSPI tft;

namespace {
class TftGfx : public Gfx {
public:
    int width() const override { return tft.width(); }
    int height() const override { return tft.height(); }
    void fillRect(int x, int y, int w, int h, uint16_t c) override { tft.fillRect(x, y, w, h, c); }
    void fillRoundRect(int x, int y, int w, int h, int r, uint16_t c) override { tft.fillRoundRect(x, y, w, h, r, c); }
    void drawRoundRect(int x, int y, int w, int h, int r, uint16_t c) override { tft.drawRoundRect(x, y, w, h, r, c); }
    void fillCircle(int x, int y, int r, uint16_t c) override { tft.fillCircle(x, y, r, c); }
    void drawCircle(int x, int y, int r, uint16_t c) override { tft.drawCircle(x, y, r, c); }
    void drawLine(int x0, int y0, int x1, int y1, uint16_t c) override { tft.drawLine(x0, y0, x1, y1, c); }
    void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t c) override {
        tft.fillTriangle(x0, y0, x1, y1, x2, y2, c);
    }
    void blit(int x, int y, int w, int h, const uint16_t *px) override {
        tft.pushImage(x, y, w, h, const_cast<uint16_t *>(px));
    }
};
TftGfx g_gfx;
}  // namespace

Gfx &gfx() { return g_gfx; }

void gfx_init(bool flipped) {
    tft.init();
    tft.setSwapBytes(true);          // our buffers are native RGB565, the panel wants big endian
    tft.setRotation(flipped ? 3 : 1);
    tft.fillScreen(TFT_BLACK);
}

void gfx_set_flipped(bool flipped) {
    tft.setRotation(flipped ? 3 : 1);
    tft.fillScreen(TFT_BLACK);
}
