#pragma once
#include <stdint.h>

// 4-bit alpha glyph, row-major, two pixels per byte (high nibble first), no row padding.
struct GlyphDef {
    uint32_t off;    // byte offset in the face bitmap
    uint16_t cp;     // Unicode code point
    uint8_t w, h;    // bitmap size
    int8_t xoff;     // left edge relative to the pen position
    int8_t yoff;     // top edge relative to the baseline (negative = above)
    uint8_t adv;     // pen advance
};

struct TextFont {
    const GlyphDef *glyphs;  // sorted by code point
    uint16_t count;
    const uint8_t *bits;
    uint8_t ascent;
    uint8_t lineHeight;
};

struct DigitGlyph {
    uint32_t off;
    uint8_t w, h;
    int16_t xoff;    // left edge inside the cell
    int16_t yoff;    // top edge inside the box
    uint8_t adv;
};

// Digits 0-9 baked at boxH pixels tall; the renderer only ever scales DOWN.
struct DigitFont {
    const char *name;   // shown in the settings list (UTF-8)
    uint16_t boxH;
    uint16_t cellW;     // tabular cell width at boxH
    uint8_t dotPct;     // colon dot diameter, % of box height
    const DigitGlyph *glyphs;  // [10]
    const uint8_t *bits;
};

// Text faces: R = regular, B = bold, number = pixel size.
enum TextFace { TF_R12, TF_R14, TF_R18, TF_R24, TF_B14, TF_B18, TF_B24, TF_B32, TF_UI_COUNT,
                TF_DATE_BASE = TF_UI_COUNT };

// Date-line faces: kDateFontCount families x 4 sizes (14, 18, 24, 32 px), bold, subset glyph set.
inline TextFace dateFace(int family, int sizeIndex) {
    return (TextFace)(TF_DATE_BASE + family * 4 + sizeIndex);
}

extern const TextFont *const kTextFonts[];
extern const DigitFont *const kDigitFonts[];
extern const int kDigitFontCount;   // procedural LED styles follow: kDigitFontCount (upright), +1 (slanted)
extern const int kDateFontCount;
extern const char *const kDateFontNames[];
constexpr int kLedStyles = 2;
