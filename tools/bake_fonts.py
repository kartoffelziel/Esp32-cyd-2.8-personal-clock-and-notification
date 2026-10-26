#!/usr/bin/env python3
"""Bakes the fonts used by CYD Clock into src/fonts_data.cpp (4-bit alpha bitmaps).

Text faces come from DejaVu (complete Vietnamese). Clock digit faces come from
open-source (SIL OFL) fonts. Re-run only when you want to change the font set:

    python3 tools/bake_fonts.py
"""
import os
from PIL import Image, ImageDraw, ImageFont

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src", "fonts_data.cpp")
DEJAVU = "/usr/share/fonts/truetype/dejavu/"
OFL = "/mnt/skills/examples/canvas-design/canvas-fonts/"

TEXT_FACES = [  # (symbol, ttf, pixel size)
    ("R12", DEJAVU + "DejaVuSans.ttf", 12),
    ("R14", DEJAVU + "DejaVuSans.ttf", 14),
    ("R18", DEJAVU + "DejaVuSans.ttf", 18),
    ("R24", DEJAVU + "DejaVuSans.ttf", 24),
    ("B14", DEJAVU + "DejaVuSans-Bold.ttf", 14),
    ("B18", DEJAVU + "DejaVuSans-Bold.ttf", 18),
    ("B24", DEJAVU + "DejaVuSans-Bold.ttf", 24),
    ("B32", DEJAVU + "DejaVuSans-Bold.ttf", 32),
]
DIGIT_FACES = [  # (symbol, display name, ttf, colon dot diameter as % of box height)
    ("Outfit", "Hiện đại", OFL + "Outfit-Bold.ttf", 8),
    ("Mono", "JetBrains Mono", OFL + "JetBrainsMono-Bold.ttf", 8),
    ("Narrow", "Hẹp", OFL + "BigShoulders-Bold.ttf", 7),
    ("Tech", "Công nghệ", OFL + "Jura-Medium.ttf", 6),
    ("Serif", "Serif", OFL + "Gloock-Regular.ttf", 7),
    ("Thin", "Thanh mảnh", OFL + "PoiretOne-Regular.ttf", 4),
    ("Pixel", "Pixel", OFL + "PixelifySans-Medium.ttf", 8),
    # Free look-alikes of proprietary faces (the originals cannot be redistributed):
    ("Arial", "Arial (Liberation Sans)", "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf", 8),
    ("Helvetica", "Helvetica (TeX Gyre Heros)", "/usr/share/texmf/fonts/opentype/public/tex-gyre/texgyreheros-bold.otf", 8),
    ("Calibri", "Calibri (Carlito)", "/usr/share/fonts/truetype/crosextra/Carlito-Bold.ttf", 8),
]
DATE_FAMILIES = [  # (display name, ttf) - bold faces for the date line, 4 sizes each
    ("Sans (DejaVu)", DEJAVU + "DejaVuSans-Bold.ttf"),
    ("Arial (Liberation)", "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf"),
    ("Helvetica (TeX Gyre)", "/usr/share/texmf/fonts/opentype/public/tex-gyre/texgyreheros-bold.otf"),
    ("Calibri (Carlito)", "/usr/share/fonts/truetype/crosextra/Carlito-Bold.ttf"),
    ("JetBrains Mono", OFL + "JetBrainsMono-Bold.ttf"),
    ("Serif (Lora)", OFL + "Lora-Bold.ttf"),
]
DATE_SIZES = [14, 18, 24, 32]
DATE_EXTRA = "ủậứưăáảàãạâấầẩẫđêếềểễệôốồổỗộơớờởỡợ"
BOX_H = 150

def date_codepoints():
    return sorted(set(list(range(0x20, 0x7F)) + [ord(c) for c in DATE_EXTRA]))

def codepoints():
    cps = list(range(0x20, 0x7F)) + list(range(0xA0, 0x100)) + list(range(0x100, 0x180))
    cps += [0x1A0, 0x1A1, 0x1AF, 0x1B0] + list(range(0x1EA0, 0x1EFA))
    cps += [0x2013, 0x2014, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2026, 0x20AB]
    return sorted(set(cps))

def render(font, ch, pad=40):
    """Returns (cropped image or None, left rel. pen origin, top rel. baseline, advance)."""
    size = font.size
    W = int(size * 3) + 2 * pad
    H = int(size * 3) + 2 * pad
    base = int(size * 2)
    im = Image.new("L", (W, H), 0)
    ImageDraw.Draw(im).text((pad, base), ch, font=font, fill=255, anchor="ls")
    bb = im.getbbox()
    adv = font.getlength(ch)
    if bb is None:
        return None, 0, 0, adv
    return im.crop(bb), bb[0] - pad, bb[1] - base, adv

def pack4(img):
    w, h = img.size
    px = img.load()
    vals = [min(15, (px[x, y] + 8) // 17) for y in range(h) for x in range(w)]
    if len(vals) % 2:
        vals.append(0)
    return bytes((vals[i] << 4) | vals[i + 1] for i in range(0, len(vals), 2))

def c_array(name, data):
    lines, row = [], []
    for b in data:
        row.append("%d" % b)
        if len(row) == 32:
            lines.append(",".join(row)); row = []
    if row:
        lines.append(",".join(row))
    return "static const uint8_t %s[] = {\n%s\n};\n" % (name, ",\n".join(lines))

def bake_text(ttf, size, cps=None):
    font = ImageFont.truetype(ttf, size)
    metas, blob = [], bytearray()
    top_min, bottom_max = 0, 0
    for cp in (cps or codepoints()):
        img, left, top, adv = render(font, chr(cp))
        if img is None:
            if cp == 0x20 or cp == 0xA0:
                metas.append((cp, 0, 0, 0, 0, round(adv), len(blob)))
            continue
        w, h = img.size
        metas.append((cp, w, h, left, top, round(adv), len(blob)))
        blob += pack4(img)
        top_min = min(top_min, top)            # tallest glyph (accented capitals) sets the ascent
        if chr(cp) in "gjpqyỵ":
            bottom_max = max(bottom_max, top + h)
    ascent = -top_min
    line = ascent + bottom_max + 1
    return metas, bytes(blob), ascent, line

def bake_digits(ttf):
    size = 400
    for _ in range(4):
        font = ImageFont.truetype(ttf, size)
        rs = [render(font, str(d)) for d in range(10)]
        h = max(r[2] + r[0].size[1] for r in rs) - min(r[2] for r in rs)
        if abs(h - BOX_H) <= 1:
            break
        size = max(8, int(round(size * BOX_H / h)))
    font = ImageFont.truetype(ttf, size)
    rs = [render(font, str(d)) for d in range(10)]
    top = min(r[2] for r in rs)
    box_h = max(r[2] + r[0].size[1] for r in rs) - top
    # Tabular cells from the INK width (not the font advance, which carries wide side bearings),
    # so "HH:MM" gets as large as the 320 px screen allows. Each digit is centred in its cell.
    cell = max(r[0].size[0] for r in rs) + int(round(0.07 * box_h))
    metas, blob = [], bytearray()
    for img, left, t, adv in rs:
        w, h = img.size
        metas.append((len(blob), w, h, (cell - w) // 2, t - top, cell))
        blob += pack4(img)
    return metas, bytes(blob), box_h, cell

def main():
    out = ['// GENERATED by tools/bake_fonts.py - do not edit by hand.',
           '// Text: DejaVu Sans (Bitstream Vera / DejaVu licence). Digits: SIL Open Font Licence fonts (see licenses/).',
           '#include "fonts.h"', '']
    for sym, ttf, size in TEXT_FACES:
        metas, blob, asc, line = bake_text(ttf, size)
        out.append(c_array("TF_%s_BITS" % sym, blob))
        out.append("static const GlyphDef TF_%s_GLYPHS[] = {" % sym)
        for cp, w, h, xo, yo, adv, off in metas:
            out.append("  {%d,%d,%d,%d,%d,%d,%d}," % (off, cp, w, h, xo, yo, adv))
        out.append("};")
        out.append("static const TextFont kText_%s = {TF_%s_GLYPHS, %d, TF_%s_BITS, %d, %d};\n" % (sym, sym, len(metas), sym, asc, line))
        print("text %-4s glyphs=%d bytes=%d asc=%d line=%d" % (sym, len(metas), len(blob), asc, line))
    for sym, disp, ttf, dot in DIGIT_FACES:
        metas, blob, box_h, cell = bake_digits(ttf)
        out.append(c_array("DF_%s_BITS" % sym, blob))
        out.append("static const DigitGlyph DF_%s_GLYPHS[10] = {" % sym)
        for off, w, h, xo, yo, adv in metas:
            out.append("  {%d,%d,%d,%d,%d,%d}," % (off, w, h, xo, yo, adv))
        out.append("};")
        out.append('static const DigitFont kDigit_%s = {"%s", %d, %d, %d, DF_%s_GLYPHS, DF_%s_BITS};\n' % (sym, disp, box_h, cell, dot, sym, sym))
        print("digit %-7s box=%d cell=%d bytes=%d" % (sym, box_h, cell, len(blob)))
    date_syms = []
    for fi, (disp, ttf) in enumerate(DATE_FAMILIES):
        for si, size in enumerate(DATE_SIZES):
            sym = "D%d_%d" % (fi, size)
            metas, blob, asc, line = bake_text(ttf, size, date_codepoints())
            out.append(c_array("TF_%s_BITS" % sym, blob))
            out.append("static const GlyphDef TF_%s_GLYPHS[] = {" % sym)
            for cp, w, h, xo, yo, adv, off in metas:
                out.append("  {%d,%d,%d,%d,%d,%d,%d}," % (off, cp, w, h, xo, yo, adv))
            out.append("};")
            out.append("static const TextFont kText_%s = {TF_%s_GLYPHS, %d, TF_%s_BITS, %d, %d};\n" % (sym, sym, len(metas), sym, asc, line))
            date_syms.append(sym)
        print("date family %-22s ok" % disp)
    all_syms = [s_ for s_, _, _ in TEXT_FACES] + date_syms
    out.append("const TextFont *const kTextFonts[] = {" + ", ".join("&kText_%s" % s_ for s_ in all_syms) + "};")
    out.append("const DigitFont *const kDigitFonts[] = {" + ", ".join("&kDigit_%s" % s_ for s_, _, _, _ in DIGIT_FACES) + "};")
    out.append("const int kDigitFontCount = %d;" % len(DIGIT_FACES))
    out.append("const int kDateFontCount = %d;" % len(DATE_FAMILIES))
    out.append("const char *const kDateFontNames[] = {" + ", ".join('"%s"' % d for d, _ in DATE_FAMILIES) + "};")
    with open(OUT, "w", encoding="utf-8") as f:
        f.write("\n".join(out) + "\n")
    print("wrote", os.path.abspath(OUT), os.path.getsize(OUT) // 1024, "KB")

if __name__ == "__main__":
    main()
