#include "host_gfx.h"
#include "../src/text.h"
#include <stdio.h>
static HostGfx g_gfx;
Gfx &gfx() { return g_gfx; }
int main() {
    const Bg black = Bg::solid(0);
    const int total = kDigitFontCount + kLedStyles;
    for (int page = 0; page < 2; ++page) {
        g_gfx.fillRect(0, 0, 320, 240, 0);
        if (page == 0) {
            int y = 2;
            for (int f = 0; f < total && y < 230; ++f) {
                const int h = 17;
                int x = 4;
                const int cell = digitCellWidth(f, h);
                for (int d = 0; d < 10; ++d) { drawDigit(f, h, d, x, y, rgb(255, 255, 255), 255, black); x += cell; }
                drawColon(f, h, true, x, y, rgb(120, 220, 255), 255, black);
                x += colonCellWidth(f, h) + 6;
                const char *name = f < kDigitFontCount ? kDigitFonts[f]->name : (f == kDigitFontCount ? "LED 7" : "Digital-7");
                drawText(TF_R12, x, y + 2, name, rgb(170, 176, 190), black);
                y += 19;
            }
        } else {
            // big LED + slanted LED clock, and the date families
            int h = 80;
            for (int st = 0; st < 2; ++st) {
                const int f = kDigitFontCount + st;
                const int cell = digitCellWidth(f, h), colon = colonCellWidth(f, h);
                int x = 8, y = 2 + st * 86;
                const int vals[4] = {1, 8, 2, 5};
                drawDigit(f, h, vals[0], x, y, rgb(255, 80, 60), 255, black); x += cell;
                drawDigit(f, h, vals[1], x, y, rgb(255, 80, 60), 255, black); x += cell;
                drawColon(f, h, true, x, y, rgb(255, 80, 60), 255, black); x += colon;
                drawDigit(f, h, vals[2], x, y, rgb(255, 80, 60), 255, black); x += cell;
                drawDigit(f, h, vals[3], x, y, rgb(255, 80, 60), 255, black);
            }
            g_gfx.fillRect(0, 170, 320, 70, 0);
            int y = 172;
            for (int fam = 0; fam < 3; ++fam) {
                drawText(dateFace(fam, 1), 6, y, "Thứ sáu, 24/10/2025", rgb(255, 255, 255), black);
                drawText(TF_R12, 200, y + 3, kDateFontNames[fam], rgb(170, 176, 190), black);
                y += 22;
            }
        }
        if (page == 1) {   // second sample strip for the other three families on its own page
            char p2[64]; snprintf(p2, sizeof p2, "/tmp/fonts_%d.raw", 1); g_gfx.dump(p2);
            g_gfx.fillRect(0, 0, 320, 240, 0);
            int yy = 4;
            for (int fam = 0; fam < kDateFontCount; ++fam) {
                drawText(dateFace(fam, 2), 6, yy, "Chủ nhật, 01/11/2025", rgb(255, 255, 255), black);
                drawText(TF_R12, 6, yy + 30, kDateFontNames[fam], rgb(170, 176, 190), black);
                yy += 39;
            }
            g_gfx.dump("/tmp/fonts_2.raw");
            continue;
        }
        char p[64]; snprintf(p, sizeof p, "/tmp/fonts_%d.raw", page);
        g_gfx.dump(p);
    }
    printf("ok\n");
}
