#!/usr/bin/env python3
"""Convert 32x32 4-bit .ico files to C arrays for FRANK OS dialog icons.

Windows 95 .ico files use a different palette index ordering than CGA.
This script reads the .ico, extracts the 32x32 4bpp image, applies the
AND mask (transparent pixels -> 0xFF), and remaps palette indices from
Windows ordering to CGA ordering.

Usage:
    python3 convert_ico.py <input.ico> <array_name>

Outputs a C array to stdout.
"""

import struct
import sys

# Windows 95 palette index -> CGA palette index remap table.
# Win95 order: 0=black, 1=maroon, 2=green, 3=olive, 4=navy, 5=purple,
#              6=teal, 7=silver, 8=gray, 9=red, 10=lime, 11=yellow,
#              12=blue, 13=fuchsia, 14=aqua, 15=white
# CGA order:   0=black, 1=blue, 2=green, 3=cyan, 4=red, 5=magenta,
#              6=brown, 7=light_gray, 8=dark_gray, 9=light_blue,
#              10=light_green, 11=light_cyan, 12=light_red, 13=light_magenta,
#              14=yellow, 15=white
WIN_TO_CGA = [0, 4, 2, 6, 1, 5, 3, 7, 8, 12, 10, 14, 9, 13, 11, 15]


def read_ico(filename):
    """Read a .ico file and extract the 32x32 4bpp image."""
    with open(filename, "rb") as f:
        data = f.read()

    # ICO header: reserved(2), type(2), count(2)
    reserved, ico_type, count = struct.unpack_from("<HHH", data, 0)
    if ico_type != 1:
        raise ValueError(f"Not an icon file (type={ico_type})")

    # Find the 32x32 entry
    best_entry = None
    for i in range(count):
        off = 6 + i * 16
        width, height, color_count, reserved2 = struct.unpack_from("<BBBB", data, off)
        planes, bpp, size, offset = struct.unpack_from("<HHII", data, off + 4)

        # width/height of 0 means 256
        w = width if width != 0 else 256
        h = height if height != 0 else 256

        if w == 32 and h == 32:
            best_entry = (size, offset, color_count, bpp)
            break

    if best_entry is None:
        raise ValueError("No 32x32 image found in .ico file")

    img_size, img_offset, color_count, bpp_dir = best_entry

    # Read BITMAPINFOHEADER at img_offset
    (hdr_size, bmp_w, bmp_h, planes, bpp, compression,
     img_data_size, xppm, yppm, colors_used, colors_important) = \
        struct.unpack_from("<IiiHHIIiiII", data, img_offset)

    # bmp_h is doubled (includes AND mask height)
    actual_h = bmp_h // 2

    if bpp != 4:
        raise ValueError(f"Expected 4bpp, got {bpp}bpp")
    if actual_h != 32 or bmp_w != 32:
        raise ValueError(f"Expected 32x32, got {bmp_w}x{actual_h}")

    # Palette follows the header (16 BGRA entries for 4bpp)
    palette_offset = img_offset + hdr_size
    num_colors = colors_used if colors_used > 0 else 16

    # XOR mask (pixel data): 32 rows * 16 bytes/row = 512 bytes
    # BMP stores bottom-to-top
    xor_offset = palette_offset + num_colors * 4
    row_bytes = (bmp_w * bpp + 7) // 8
    # Pad to 4-byte boundary
    row_stride = (row_bytes + 3) & ~3

    # AND mask: 32 rows * 4 bytes/row = 128 bytes
    and_offset = xor_offset + row_stride * actual_h
    and_stride = ((bmp_w + 7) // 8 + 3) & ~3

    # Extract pixels
    pixels = []
    for row in range(actual_h):
        # BMP is bottom-to-top, flip to top-to-bottom
        bmp_row = actual_h - 1 - row
        row_data = data[xor_offset + bmp_row * row_stride:
                        xor_offset + bmp_row * row_stride + row_bytes]
        and_data = data[and_offset + bmp_row * and_stride:
                        and_offset + bmp_row * and_stride + ((bmp_w + 7) // 8)]

        for col in range(bmp_w):
            # 4bpp: 2 pixels per byte, high nibble first
            byte_idx = col // 2
            if col % 2 == 0:
                win_idx = (row_data[byte_idx] >> 4) & 0x0F
            else:
                win_idx = row_data[byte_idx] & 0x0F

            # Check AND mask (1 = transparent)
            and_byte = and_data[col // 8]
            and_bit = (and_byte >> (7 - (col % 8))) & 1

            if and_bit:
                pixels.append(0xFF)  # transparent
            else:
                pixels.append(WIN_TO_CGA[win_idx])

    return pixels


def format_c_array(name, pixels):
    """Format pixel data as a C array."""
    lines = []
    lines.append(f"const uint8_t {name}[1024] = {{")
    for row in range(32):
        row_data = pixels[row * 32:(row + 1) * 32]
        hex_vals = ", ".join(f"0x{v:02X}" for v in row_data)
        lines.append(f"    {hex_vals},")
    lines.append("};")
    return "\n".join(lines)


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input.ico> <array_name>", file=sys.stderr)
        sys.exit(1)

    filename = sys.argv[1]
    array_name = sys.argv[2]
    pixels = read_ico(filename)
    print(format_c_array(array_name, pixels))


if __name__ == "__main__":
    main()
