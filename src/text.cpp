#include "text.h"

#include <string.h>

namespace {

uint16_t g_strip[6144];

// Composes a w x h region row by row (background first, then rowFn paints on top) and
// blits it in strips. rowFn(absY, row, x0) gets a row whose element i is screen column x0+i.
template <class F>
void compose(int x, int y, int w, int h, const Bg &bg, F rowFn) {
    Gfx &g = gfx();
    const int x0 = x < 0 ? 0 : x;
    const int x1 = x + w > g.width() ? g.width() : x + w;
    const int y0 = y < 0 ? 0 : y;
    const int y1 = y + h > g.height() ? g.height() : y + h;
    const int cw = x1 - x0;
    if (cw <= 0 || y1 <= y0) return;
    const int rows = (int)(sizeof(g_strip) / sizeof(g_strip[0])) / cw;
    for (int ys = y0; ys < y1; ys += rows) {
        const int hh = (y1 - ys) < rows ? (y1 - ys) : rows;
        for (int r = 0; r < hh; ++r) {
            uint16_t *row = g_strip + r * cw;
            const uint16_t b = bg.at(ys + r);
            for (int i = 0; i < cw; ++i) row[i] = b;
            rowFn(ys + r, row, x0, cw);
        }
        g.blit(x0, ys, cw, hh, g_strip);
    }
}

uint32_t decodeUtf8(const char *&s) {
    const uint8_t c = (uint8_t)*s++;
    if (c < 0x80) return c;
    int extra = 0;
    uint32_t cp = 0;
    if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; extra = 1; }
    else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; extra = 2; }
    else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; extra = 3; }
    else return '?';
    for (int i = 0; i < extra; ++i) {
        if (((uint8_t)*s & 0xC0) != 0x80) return '?';
        cp = (cp << 6) | ((uint8_t)*s++ & 0x3F);
    }
    return cp;
}

const GlyphDef *findGlyph(const TextFont &f, uint32_t cp) {
    int lo = 0, hi = (int)f.count - 1;
    while (lo <= hi) {
        const int mid = (lo + hi) / 2;
        if (f.glyphs[mid].cp == cp) return &f.glyphs[mid];
        if (f.glyphs[mid].cp < cp) lo = mid + 1; else hi = mid - 1;
    }
    return nullptr;
}

const GlyphDef *glyphOrFallback(const TextFont &f, uint32_t cp) {
    const GlyphDef *g = findGlyph(f, cp);
    if (!g) g = findGlyph(f, '?');
    return g;
}

inline int nibble(const uint8_t *bits, uint32_t index) {
    const uint8_t b = bits[index >> 1];
    return (index & 1U) ? (b & 0x0F) : (b >> 4);
}

struct Placed {
    const GlyphDef *g;
    int x;  // pen position relative to the text origin
};
constexpr int kMaxPlaced = 160;

int layout(const TextFont &f, const char *s, Placed *out, int cap) {
    int n = 0, pen = 0;
    while (*s && n < cap) {
        const GlyphDef *g = glyphOrFallback(f, decodeUtf8(s));
        if (!g) continue;
        out[n].g = g;
        out[n].x = pen;
        ++n;
        pen += g->adv;
    }
    return n;
}

void paintText(const TextFont &f, int x, int y, const char *s, uint16_t color, const Bg &bg,
               int clipX, int clipW) {
    Placed placed[kMaxPlaced];
    const int n = layout(f, s, placed, kMaxPlaced);
    if (n == 0) return;
    int width = 0;
    for (int i = 0; i < n; ++i) width = placed[i].x + placed[i].g->adv;
    int left = x, right = x + width;
    if (clipW > 0) {
        left = left < clipX ? clipX : left;
        right = right > clipX + clipW ? clipX + clipW : right;
    }
    if (right <= left) return;
    compose(left, y, right - left, f.lineHeight, bg, [&](int absY, uint16_t *row, int x0, int cw) {
        const int ry = absY - y - f.ascent;  // row relative to the baseline
        for (int i = 0; i < n; ++i) {
            const GlyphDef &g = *placed[i].g;
            if (g.w == 0 || ry < g.yoff || ry >= g.yoff + g.h) continue;
            const int gx = x + placed[i].x + g.xoff;
            const uint8_t *bits = f.bits + g.off;                     // glyph bitmaps are byte-aligned
            const uint32_t base = (uint32_t)(ry - g.yoff) * g.w;
            for (int px = 0; px < g.w; ++px) {
                const int col = gx + px - x0;
                if (col < 0 || col >= cw) continue;
                const int a = nibble(bits, base + px);
                if (a) row[col] = mix565(row[col], color, a * 17);
            }
        }
    });
}

}  // namespace

int textWidth(TextFace face, const char *s) {
    const TextFont &f = *kTextFonts[face];
    int pen = 0;
    while (*s) {
        const GlyphDef *g = glyphOrFallback(f, decodeUtf8(s));
        if (g) pen += g->adv;
    }
    return pen;
}

int textHeight(TextFace face) { return kTextFonts[face]->lineHeight; }

void drawText(TextFace face, int x, int y, const char *s, uint16_t color, const Bg &bg) {
    paintText(*kTextFonts[face], x, y, s, color, bg, 0, 0);
}

void drawTextCentered(TextFace face, int cx, int y, const char *s, uint16_t color, const Bg &bg) {
    drawText(face, cx - textWidth(face, s) / 2, y, s, color, bg);
}

void drawTextClipped(TextFace face, int x, int y, const char *s, uint16_t color, const Bg &bg,
                     int clipX, int clipW) {
    paintText(*kTextFonts[face], x, y, s, color, bg, clipX, clipW);
}

void drawTextInBand(TextFace face, int textX, int bandX, int bandY, int bandW, int bandH,
                    const char *s, uint16_t color, const Bg &bg) {
    const TextFont &f = *kTextFonts[face];
    Placed placed[kMaxPlaced];
    const int n = layout(f, s, placed, kMaxPlaced);
    const int textY = bandY + (bandH - f.lineHeight) / 2;
    compose(bandX, bandY, bandW, bandH, bg, [&](int absY, uint16_t *row, int x0, int cw) {
        const int ry = absY - textY - f.ascent;
        for (int i = 0; i < n; ++i) {
            const GlyphDef &g = *placed[i].g;
            if (g.w == 0 || ry < g.yoff || ry >= g.yoff + g.h) continue;
            const int gx = textX + placed[i].x + g.xoff;
            if (gx + g.w <= x0 || gx >= x0 + cw) continue;
            const uint8_t *bits = f.bits + g.off;
            const uint32_t base = (uint32_t)(ry - g.yoff) * g.w;
            for (int px = 0; px < g.w; ++px) {
                const int col = gx + px - x0;
                if (col < 0 || col >= cw) continue;
                const int a = nibble(bits, base + px);
                if (a) row[col] = mix565(row[col], color, a * 17);
            }
        }
    });
}

void fitText(TextFace face, char *s, int maxW) {
    if (textWidth(face, s) <= maxW) return;
    const int dots = textWidth(face, "...");
    int len = (int)strlen(s);
    while (len > 0) {
        // step back to the previous UTF-8 character boundary
        --len;
        while (len > 0 && (((uint8_t)s[len]) & 0xC0) == 0x80) --len;
        s[len] = 0;
        if (textWidth(face, s) + dots <= maxW) break;
    }
    strcat(s, "...");
}

// ------------------------------------------------------------------------------------------
// Clock digits
// ------------------------------------------------------------------------------------------
namespace {

bool isLed(int fontIndex) { return fontIndex >= kDigitFontCount; }
bool ledSlanted(int fontIndex) { return fontIndex - kDigitFontCount == 1; }

const DigitFont &bakedFont(int fontIndex) {
    return *kDigitFonts[fontIndex < 0 ? 0 : (fontIndex >= kDigitFontCount ? kDigitFontCount - 1 : fontIndex)];
}

int clampH(int fontIndex, int height) {
    int maxH = isLed(fontIndex) ? 200 : bakedFont(fontIndex).boxH;
    if (height > maxH) height = maxH;
    return height < 16 ? 16 : height;
}

struct Span {
    int16_t first;
    uint8_t n;
    uint16_t w[5];  // 0..256, sums to ~256
};

// Box-filter spans: destination pixel d covers source [d*per16, (d+1)*per16) (16.16 fixed point,
// per16 >= 1.0 so we only ever scale down). Weights are 0..256 and sum to about 256.
int buildSpans(Span *spans, int dst, int per16, int maxCount) {
    int count = 0;
    for (int d = 0; d < dst && count < maxCount; ++d, ++count) {
        const int64_t s0 = (int64_t)d * per16;
        const int64_t s1 = (int64_t)(d + 1) * per16;
        Span &sp = spans[count];
        sp.first = (int16_t)(s0 >> 16);
        const int last = (int)((s1 - 1) >> 16);
        int n = last - sp.first + 1;
        if (n > 5) n = 5;
        sp.n = (uint8_t)n;
        int sum = 0;
        for (int i = 0; i < n; ++i) {
            const int64_t a = (int64_t)(sp.first + i) << 16;
            const int64_t lo = s0 > a ? s0 : a;
            const int64_t hi = s1 < a + 65536 ? s1 : a + 65536;
            const int64_t ov = hi > lo ? hi - lo : 0;
            sp.w[i] = (uint16_t)(ov * 256 / per16);
            sum += sp.w[i];
        }
        if (sum == 0) sp.w[0] = 256;
    }
    return count;
}

Span g_colSpans[200];

// Seven-segment digits with hexagonal (pointed) segments. The slanted variant imitates the look of
// the classic "Digital-7 italic" fonts without using that (personal-use only) font.
void paintLedDigit(int digit, int height, int cellW, bool slant, int x, int y, uint16_t color,
                   int strength, const Bg &bg) {
    static const uint8_t kSeg[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
    const int H = height;
    const int sh = slant ? H * 14 / 100 : 0;
    const int pad = H * 12 / 100 / 2;       // half of the gap between neighbouring digits
    const int bw = cellW - sh - 2 * pad;
    int t = H * 13 / 100;
    if (t < 3) t = 3;
    const int gap = t / 5 < 1 ? 1 : t / 5;
    const int mid = H / 2;
    struct Seg { int x0, y0, x1, y1; bool horiz; };
    const Seg segs[7] = {
        {t / 2 + gap, 0, bw - t / 2 - gap, t, true},                          // a
        {bw - t, t / 2 + gap, bw, mid - gap, false},                          // b
        {bw - t, mid + gap, bw, H - t / 2 - gap, false},                      // c
        {t / 2 + gap, H - t, bw - t / 2 - gap, H, true},                      // d
        {0, mid + gap, t, H - t / 2 - gap, false},                            // e
        {0, t / 2 + gap, t, mid - gap, false},                                // f
        {t / 2 + gap, mid - t / 2, bw - t / 2 - gap, mid - t / 2 + t, true},  // g
    };
    const uint8_t mask = (digit >= 0 && digit <= 9) ? kSeg[digit] : (digit == -2 ? 0x40 : 0);
    compose(x, y, cellW, H, bg, [&](int absY, uint16_t *row, int x0, int cw) {
        const int ry = absY - y;
        const int shift = slant ? (H - 1 - ry) * sh / H : 0;
        for (int sIdx = 0; sIdx < 7; ++sIdx) {
            if (!(mask & (1 << sIdx))) continue;
            const Seg &sg = segs[sIdx];
            if (ry < sg.y0 || ry >= sg.y1) continue;
            int inset;
            if (sg.horiz) {
                const int d = 2 * (ry - sg.y0) - (t - 1);
                inset = (d < 0 ? -d : d) / 2;
            } else {
                const int e1 = ry - sg.y0, e2 = sg.y1 - 1 - ry;
                const int e = e1 < e2 ? e1 : e2;
                inset = t / 2 - e > 0 ? t / 2 - e : 0;
            }
            for (int px = sg.x0 + inset; px < sg.x1 - inset; ++px) {
                const int col = x + pad + px + shift - x0;
                if (col >= 0 && col < cw) row[col] = mix565(row[col], color, strength);
            }
        }
    });
}

}  // namespace

int digitCellWidth(int fontIndex, int height) {
    const int H = clampH(fontIndex, height);
    if (isLed(fontIndex)) return H * 50 / 100 + H * 12 / 100 + (ledSlanted(fontIndex) ? H * 14 / 100 : 0) + 1;
    const DigitFont &f = bakedFont(fontIndex);
    return (f.cellW * H + f.boxH / 2) / f.boxH;
}

int colonCellWidth(int fontIndex, int height) {
    const int H = clampH(fontIndex, height);
    return isLed(fontIndex) ? H * 30 / 100 : H * 36 / 100;
}

int maxClockHeight(int fontIndex, int screenWidth, int digits) {
    const int cap = isLed(fontIndex) ? 200 : bakedFont(fontIndex).boxH;
    int best = 24;
    for (int h = 24; h <= cap; ++h) {
        const int w = digits * digitCellWidth(fontIndex, h) + colonCellWidth(fontIndex, h);
        if (w <= screenWidth - 8) best = h; else break;
    }
    return best;
}

void drawDigit(int fontIndex, int height, int digit, int x, int y, uint16_t color, int strength,
               const Bg &bg) {
    const int H = clampH(fontIndex, height);
    const int W = digitCellWidth(fontIndex, H);
    if (strength > 255) strength = 255;
    if (strength < 0) strength = 0;
    if (isLed(fontIndex)) {
        paintLedDigit(digit, H, W, ledSlanted(fontIndex), x, y, color, strength, bg);
        return;
    }
    const DigitFont &f = bakedFont(fontIndex);
    if (digit < 0 || digit > 9) {
        const int barH = H * 8 / 100 < 2 ? 2 : H * 8 / 100;
        compose(x, y, W, H, bg, [&](int absY, uint16_t *row, int x0, int cw) {
            const int ry = absY - y;
            if (digit != -2 || ry < H / 2 - barH / 2 || ry >= H / 2 - barH / 2 + barH) return;
            for (int px = W / 4; px < W - W / 4; ++px) {
                const int col = x + px - x0;
                if (col >= 0 && col < cw) row[col] = mix565(row[col], color, strength);
            }
        });
        return;
    }
    const DigitGlyph &gl = f.glyphs[digit];
    // source pixels per destination pixel, 16.16 fixed point
    const int per16 = (int)(((int64_t)f.boxH << 16) / H);
    Span rows[160];
    const int nRows = buildSpans(rows, H, per16, 160);
    const int nCols = buildSpans(g_colSpans, W, per16, 200);
    compose(x, y, W, H, bg, [&](int absY, uint16_t *row, int x0, int cw) {
        const int dy = absY - y;
        if (dy < 0 || dy >= nRows) return;
        const Span &sy = rows[dy];
        for (int dx = 0; dx < nCols; ++dx) {
            const Span &sx = g_colSpans[dx];
            int acc = 0;
            for (int j = 0; j < sy.n; ++j) {
                const int gy = sy.first + j - gl.yoff;
                if (gy < 0 || gy >= gl.h) continue;
                int line = 0;
                for (int i = 0; i < sx.n; ++i) {
                    const int gx = sx.first + i - gl.xoff;
                    if (gx < 0 || gx >= gl.w) continue;
                    line += sx.w[i] * nibble(f.bits + gl.off, (uint32_t)gy * gl.w + gx);
                }
                acc += sy.w[j] * line;
            }
            int alpha = acc * 17 / 65536;           // 0..255
            alpha = alpha * strength / 255;
            if (alpha <= 0) continue;
            const int col = x + dx - x0;
            if (col >= 0 && col < cw) row[col] = mix565(row[col], color, alpha > 255 ? 255 : alpha);
        }
    });
}

void drawColon(int fontIndex, int height, bool on, int x, int y, uint16_t color, int strength,
               const Bg &bg) {
    const int H = clampH(fontIndex, height);
    const int W = colonCellWidth(fontIndex, H);
    if (strength > 255) strength = 255;
    int dot;
    if (isLed(fontIndex)) dot = H * 14 / 100 < 3 ? 3 : H * 14 / 100;
    else dot = H * bakedFont(fontIndex).dotPct / 100 < 3 ? 3 : H * bakedFont(fontIndex).dotPct / 100;
    const float r = dot / 2.0f;
    const float cx = W / 2.0f;
    const float cy[2] = {H * 0.30f, H * 0.70f};
    const float slantShift[2] = {isLed(fontIndex) && ledSlanted(fontIndex) ? H * 0.14f * 0.30f : 0.0f,
                                 isLed(fontIndex) && ledSlanted(fontIndex) ? -H * 0.14f * 0.30f : 0.0f};
    compose(x, y, W, H, bg, [&](int absY, uint16_t *row, int x0, int cw) {
        if (!on) return;
        const float py = (absY - y) + 0.5f;
        for (int k = 0; k < 2; ++k) {
            const float dy = py - cy[k];
            if (dy > r + 1 || dy < -r - 1) continue;
            const int xs = (int)(cx + slantShift[k] - r - 1), xe = (int)(cx + slantShift[k] + r + 2);
            for (int px = xs; px <= xe; ++px) {
                const float dx = (px + 0.5f) - (cx + slantShift[k]);
                float d2 = dx * dx + dy * dy;
                float dist = 0;
                // sqrt without <cmath> dependence on target: Newton steps
                if (d2 > 0) { dist = d2 > 1 ? d2 / 2 : 0.5f; for (int i = 0; i < 6; ++i) dist = 0.5f * (dist + d2 / dist); }
                float cov = r + 0.5f - dist;
                cov = cov < 0 ? 0 : (cov > 1 ? 1 : cov);
                const int a = (int)(cov * strength);
                const int col = x + px - x0;
                if (a > 0 && col >= 0 && col < cw) row[col] = mix565(row[col], color, a);
            }
        }
    });
}
