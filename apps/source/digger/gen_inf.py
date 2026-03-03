#!/usr/bin/env python3
"""Generate digger.inf from raw icon data (16x16, upscaled to 32x32).

INF v2 format:
    <display name>\n
    <256 bytes: 16x16 raw icon>
    <1024 bytes: 32x32 upscaled icon>

Usage:
    python3 gen_inf.py <display_name> <icon.raw> <output.inf>
"""
import sys


def nn_upscale_16_to_32(raw16):
    """Nearest-neighbour upscale 16x16 -> 32x32."""
    out = []
    for y in range(32):
        sy = y * 16 // 32
        for x in range(32):
            sx = x * 16 // 32
            out.append(raw16[sy * 16 + sx])
    return bytes(out)


name = sys.argv[1]
icon_path = sys.argv[2]
out_path = sys.argv[3]

with open(icon_path, 'rb') as f:
    icon16 = f.read(256)

if len(icon16) != 256:
    print(f"Error: expected 256 bytes, got {len(icon16)}", file=sys.stderr)
    sys.exit(1)

icon32 = nn_upscale_16_to_32(icon16)

with open(out_path, 'wb') as f:
    f.write(name.encode('ascii') + b'\n')
    f.write(icon16)
    f.write(icon32)
