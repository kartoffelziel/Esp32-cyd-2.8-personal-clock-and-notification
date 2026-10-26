#!/usr/bin/env python3
"""render.py out.png a.raw b.raw ...  -> contact sheet of 320x240 RGB565 dumps."""
import struct, sys
from PIL import Image
def load(path):
    data = open(path, "rb").read()
    px = struct.unpack("<" + "H" * (320 * 240), data)
    img = Image.new("RGB", (320, 240))
    img.putdata([(((p >> 11) & 31) * 255 // 31, ((p >> 5) & 63) * 255 // 63, (p & 31) * 255 // 31) for p in px])
    return img
out, files = sys.argv[1], sys.argv[2:]
cols = 2 if len(files) > 1 else 1
rows = (len(files) + cols - 1) // cols
sheet = Image.new("RGB", (cols * 330, rows * 250), (50, 50, 50))
for i, f in enumerate(files):
    sheet.paste(load(f), ((i % cols) * 330 + 5, (i // cols) * 250 + 5))
sheet.save(out)
