#!/usr/bin/env python3
"""Crop and upscale a region of the rendered preview for close inspection."""
import sys
from PIL import Image

src = sys.argv[1]
dst = sys.argv[2]
box = [float(v) for v in sys.argv[3:7]]  # x0 y0 x1 y1 in pixels of the source
scale = float(sys.argv[7]) if len(sys.argv) > 7 else 2.0

im = Image.open(src).convert("RGB")
crop = im.crop((int(box[0]), int(box[1]), int(box[2]), int(box[3])))
crop = crop.resize((int(crop.width * scale), int(crop.height * scale)), Image.LANCZOS)
crop.save(dst)
print(f"{dst} {crop.width}x{crop.height}")
