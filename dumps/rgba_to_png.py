#!/usr/bin/env python3
"""Convert the raw RGBA dump from im5_host_check --shot into a PNG."""
import struct, sys, zlib

def main():
    src, dst, w, h = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4])
    data = open(src, 'rb').read()
    assert len(data) == w * h * 4, (len(data), w * h * 4)

    # Optional crop + integer zoom for inspecting a region: src dst w h [x y cw ch zoom]
    if len(sys.argv) >= 10:
        x, y, cw, ch, zoom = (int(v) for v in sys.argv[5:10])
        cw = min(cw, w - x); ch = min(ch, h - y)
        out = bytearray()
        for yy in range(ch * zoom):
            for xx in range(cw * zoom):
                sx = x + xx // zoom
                sy = y + yy // zoom
                out += data[(sy * w + sx) * 4:(sy * w + sx) * 4 + 4]
        data = bytes(out)
        w, h = cw * zoom, ch * zoom
    raw = bytearray()
    for y in range(h):
        raw.append(0)                       # filter: none
        raw += data[y * w * 4:(y + 1) * w * 4]
    def chunk(tag, payload):
        return (struct.pack('>I', len(payload)) + tag + payload +
                struct.pack('>I', zlib.crc32(tag + payload) & 0xffffffff))
    png = b'\x89PNG\r\n\x1a\n'
    png += chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0))
    png += chunk(b'IDAT', zlib.compress(bytes(raw), 6))
    png += chunk(b'IEND', b'')
    open(dst, 'wb').write(png)
    print('wrote', dst, w, 'x', h)

main()
