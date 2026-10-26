#include "host_gfx.h"
#include "../src/text.h"
#include "../src/hal.h"
#include <stdio.h>
static HostGfx g_gfx;
Gfx &gfx() { return g_gfx; }
int main() {
    const Bg black = Bg::solid(0);
    g_gfx.fillRect(0, 0, 320, 240, 0);
    // digits: each font at 90 px (scaled down from 150), plus text sizes
    int x = 4;
    for (int f = 0; f <= kDigitFontCount; ++f) {
        int h = 56;
        int cw = digitCellWidth(f, h);
        if (x + cw * 2 > 316) break;
        drawDigit(f, h, 1 + f % 9, x, 4, rgb(255, 255, 255), 255, black);
        drawDigit(f, h, (f * 3 + 4) % 10, x + cw, 4, rgb(255, 200, 80), 255, black);
        x += cw * 2 + 6;
    }
    int cw = digitCellWidth(2, 110);
    drawDigit(2, 110, 1, 4, 70, rgb(255, 255, 255), 255, black);
    drawDigit(2, 110, 8, 4 + cw, 70, rgb(255, 255, 255), 255, black);
    drawColon(2, 110, true, 4 + 2 * cw, 70, rgb(255, 255, 255), 255, black);
    drawDigit(2, 110, 2, 4 + 2 * cw + colonCellWidth(2, 110), 70, rgb(120, 220, 255), 255, black);
    drawDigit(2, 110, 5, 4 + 3 * cw + colonCellWidth(2, 110), 70, rgb(120, 220, 255), 255, black);
    const char *vi = "Thứ sáu, 24/10/2025 Nguyễn Trãi ₫";
    drawText(TF_B32, 4, 182, "Thứ Sáu, 24/10", rgb(255, 255, 255), black);
    drawText(TF_B24, 4, 150 + 0, vi, rgb(200, 255, 200), black);
    // text ladder at right
    int y = 74;
    const TextFace faces[] = {TF_R12, TF_R14, TF_R18, TF_R24, TF_B14, TF_B18};
    for (TextFace f : faces) { drawText(f, 180, y, "Chủ nhật Thứ ba ứ ử ỗ ẫ", rgb(255, 255, 255), black); y += textHeight(f) + 1; }
    g_gfx.dump("/tmp/cs_text.raw");
    printf("ok blits=%ld\n", g_gfx.blits);
    return 0;
}
