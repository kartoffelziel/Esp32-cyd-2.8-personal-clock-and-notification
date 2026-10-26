#pragma once
#include "fonts.h"
#include "gfx.h"

// Vertical gradient background used to blend anti-aliased glyphs correctly.
struct Bg {
    uint16_t top, bottom;
    int y0, y1;                    // gradient spans y0..y1; solid when top == bottom
    static Bg solid(uint16_t c) { return Bg{c, c, 0, 1}; }
    uint16_t at(int y) const {
        if (top == bottom || y1 <= y0) return top;
        int t = (y - y0) * 255 / (y1 - y0);
        t = t < 0 ? 0 : (t > 255 ? 255 : t);
        return mix565(top, bottom, t);
    }
};

// ---- text (DejaVu, UTF-8, Vietnamese complete) --------------------------------------
int textWidth(TextFace face, const char *utf8);
int textHeight(TextFace face);
// Draws with the top of the line box at y. Only the glyph pixels' rows are composed.
void drawText(TextFace face, int x, int y, const char *utf8, uint16_t color, const Bg &bg);
void drawTextCentered(TextFace face, int cx, int y, const char *utf8, uint16_t color, const Bg &bg);
// Marquee helper: draws into a clip window [clipX, clipX+clipW) with the text starting at x (may be < clipX).
void drawTextClipped(TextFace face, int x, int y, const char *utf8, uint16_t color, const Bg &bg,
                     int clipX, int clipW);
// Flicker-free marquee: composes the whole band (bandX..bandX+bandW, bandH tall) with the text drawn at
// textX, vertically centred, in one pass.
void drawTextInBand(TextFace face, int textX, int bandX, int bandY, int bandW, int bandH,
                    const char *utf8, uint16_t color, const Bg &bg);
// Truncates in place with an ellipsis so that it fits maxW.
void fitText(TextFace face, char *utf8, int maxW);

// ---- clock digits ------------------------------------------------------------------------
// fontIndex: 0..kDigitFontCount-1 baked fonts, kDigitFontCount = procedural LED style.
int digitCellWidth(int fontIndex, int height);
int colonCellWidth(int fontIndex, int height);
int maxClockHeight(int fontIndex, int screenWidth, int digits);
// digit: 0..9, -1 = blank, -2 = "-". strength 0..255 scales the glyph opacity (fade / breathing).
void drawDigit(int fontIndex, int height, int digit, int x, int y, uint16_t color, int strength,
               const Bg &bg);
void drawColon(int fontIndex, int height, bool on, int x, int y, uint16_t color, int strength,
               const Bg &bg);
