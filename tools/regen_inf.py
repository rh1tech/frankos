#!/usr/bin/env python3
"""Regenerate all FRANK OS .inf files and deploy .ico icon files.

Apps with a .ico file in assets/apps/ get:
  - A lightweight .inf (display name + ext line only, no embedded icons)
  - The .ico file copied alongside the .inf

Apps without a .ico file keep the legacy format with embedded icon data:
  - .inf with display name + 256 bytes 16x16 + 1024 bytes 32x32 + ext line

Run from the repository root:
    python3 tools/regen_inf.py
"""

import os
import shutil
import sys
import struct

REPO_ROOT    = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ICONS_DIR    = os.path.join(REPO_ROOT, "assets", "icons")
APP_ICONS    = os.path.join(REPO_ROOT, "assets", "apps")
FOS_DIR      = os.path.join(REPO_ROOT, "sdcard", "fos")
COMPILED_DIR = os.path.join(REPO_ROOT, "apps", "compiled")

# ─── app definitions ────────────────────────────────────────────────────────
#   app_id       display name     legacy ico (from assets/icons/)   ext line
#   If assets/apps/<app_id>.ico exists, the legacy ico is ignored and the
#   .ico file is deployed instead.
APPS = [
    ("basic",       "MMBasic",      "w95_60.ico",                   "ext:bas"),
    ("digger",      "Digger",       None,                           None),
    ("frankamp",    "FrankAmp",     "w2k_mplayer32_1.ico",          "ext:mp3,mod"),
    ("minesweeper", "Minesweeper",  "mine.ico",                     None),
    ("notepad",     "Notepad",      "w2k_notepad_1.ico",            "ext:txt,log,cfg,ini,md,csv,c,h,py,sh,bas"),
    ("pshell",      "PShell",       "w2k_default_application.ico",  None),
    ("solitaire",   "Solitaire",    None,                           None),
    ("zxspectrum",  "ZX Spectrum",  None,                           "ext:z80,sna,tap"),
]

# ─── Windows→CGA palette remap ──────────────────────────────────────────────
WIN_TO_CGA = [0, 4, 2, 6, 1, 5, 3, 7, 8, 12, 10, 14, 9, 13, 11, 15]


# ─── ICO extraction helpers (legacy path only) ──────────────────────────────

def read_ico_entry(data, target_size):
    _reserved, ico_type, count = struct.unpack_from("<HHH", data, 0)
    if ico_type != 1:
        raise ValueError("Not an icon file")
    for i in range(count):
        off = 6 + i * 16
        w, h = struct.unpack_from("<BB", data, off)
        w = w or 256
        h = h or 256
        _cc, _res2 = struct.unpack_from("<BB", data, off + 2)
        _planes, _bpp, size, offset = struct.unpack_from("<HHII", data, off + 4)
        if w == target_size and h == target_size:
            return (size, offset)
    return None


def extract_4bpp(data, img_offset, expected_w):
    (hdr_size, bmp_w, bmp_h, _planes, bpp, _comp,
     _isize, _xp, _yp, colors_used, _ci) = struct.unpack_from("<IiiHHIIiiII", data, img_offset)
    actual_h = bmp_h // 2
    if bpp != 4:
        raise ValueError(f"Expected 4bpp, got {bpp}bpp")
    if actual_h != expected_w or bmp_w != expected_w:
        raise ValueError(f"Expected {expected_w}x{expected_w}, got {bmp_w}x{actual_h}")
    palette_off = img_offset + hdr_size
    num_colors  = colors_used if colors_used > 0 else 16
    row_bytes   = (bmp_w * bpp + 7) // 8
    row_stride  = (row_bytes + 3) & ~3
    xor_off     = palette_off + num_colors * 4
    and_off     = xor_off + row_stride * actual_h
    and_stride  = ((bmp_w + 7) // 8 + 3) & ~3
    pixels = []
    for row in range(actual_h):
        br   = actual_h - 1 - row
        rdat = data[xor_off + br * row_stride : xor_off + br * row_stride + row_bytes]
        adat = data[and_off  + br * and_stride : and_off  + br * and_stride + ((bmp_w + 7) // 8)]
        for col in range(bmp_w):
            bidx = col // 2
            win_idx = (rdat[bidx] >> 4) & 0x0F if col % 2 == 0 else rdat[bidx] & 0x0F
            and_bit = (adat[col // 8] >> (7 - col % 8)) & 1
            pixels.append(0xFF if and_bit else WIN_TO_CGA[win_idx])
    return pixels


def nn_scale(pixels, src, dst):
    out = []
    for y in range(dst):
        sy = y * src // dst
        for x in range(dst):
            sx = x * src // dst
            out.append(pixels[sy * src + sx])
    return out


def read_ico_16(path):
    with open(path, "rb") as f:
        data = f.read()
    entry = read_ico_entry(data, 16)
    if entry:
        return extract_4bpp(data, entry[1], 16)
    entry = read_ico_entry(data, 32)
    if entry:
        return nn_scale(extract_4bpp(data, entry[1], 32), 32, 16)
    raise ValueError(f"No 16x16 or 32x32 image in {path}")


def read_ico_32(path):
    with open(path, "rb") as f:
        data = f.read()
    entry = read_ico_entry(data, 32)
    if entry:
        return extract_4bpp(data, entry[1], 32)
    entry = read_ico_entry(data, 16)
    if entry:
        return nn_scale(extract_4bpp(data, entry[1], 16), 16, 32)
    raise ValueError(f"No 16x16 or 32x32 image in {path}")


def upscale_raw16_to_32(raw16):
    return nn_scale(list(raw16), 16, 32)


# ─── ZX Spectrum: original diagonal rainbow icon ──────────────────────────────

def get_zxspectrum_icons():
    pixels16 = [
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x0C,0x0E,0x0E,0x0E,0x0A,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x0C,0x0E,0x0E,0x0E,0x0A,0x0A,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x0C,0x0E,0x0E,0x0E,0x0A,0x0A,0x0A,
        0x00,0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x0C,0x0E,0x0E,0x0E,0x0A,0x0A,0x0A,0x0B,
        0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x0C,0x0E,0x0E,0x0E,0x0A,0x0A,0x0A,0x0B,0x0B,
        0x00,0x00,0x00,0x00,0x0C,0x0C,0x0C,0x0E,0x0E,0x0E,0x0A,0x0A,0x0A,0x0B,0x0B,0x0B,
        0x00,0x00,0x00,0x0C,0x0C,0x0C,0x0E,0x0E,0x0E,0x0A,0x0A,0x0A,0x0B,0x0B,0x0B,0x00,
        0x00,0x00,0x0C,0x0C,0x0C,0x0E,0x0E,0x0E,0x0A,0x0A,0x0A,0x0B,0x0B,0x0B,0x00,0x00,
        0x00,0x0C,0x0C,0x0C,0x0E,0x0E,0x0E,0x0A,0x0A,0x0A,0x0B,0x0B,0x0B,0x00,0x00,0x00,
        0x0C,0x0C,0x0C,0x0E,0x0E,0x0E,0x0A,0x0A,0x0A,0x0B,0x0B,0x0B,0x00,0x00,0x00,0x00,
        0x0C,0x0C,0x0E,0x0E,0x0E,0x0A,0x0A,0x0A,0x0B,0x0B,0x0B,0x00,0x00,0x00,0x00,0x00,
        0x0C,0x0E,0x0E,0x0E,0x0A,0x0A,0x0A,0x0B,0x0B,0x0B,0x00,0x00,0x00,0x00,0x00,0x00,
        0x0E,0x0E,0x0E,0x0A,0x0A,0x0A,0x0B,0x0B,0x0B,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x0E,0x0E,0x0A,0x0A,0x0A,0x0B,0x0B,0x0B,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x0E,0x0A,0x0A,0x0A,0x0B,0x0B,0x0B,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x0A,0x0A,0x0A,0x0B,0x0B,0x0B,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    ]
    pixels32 = nn_scale(pixels16, 16, 32)
    return pixels16, pixels32


# ─── Digger: custom raw icon ──────────────────────────────────────────────────

DIGGER_RAW = os.path.join(REPO_ROOT, "apps", "source", "digger", "digger_icon.raw")

def get_digger_icons():
    with open(DIGGER_RAW, "rb") as f:
        raw = f.read(256)
    if len(raw) != 256:
        raise ValueError(f"Expected 256 bytes in {DIGGER_RAW}, got {len(raw)}")
    pixels16 = list(raw)
    pixels32 = upscale_raw16_to_32(raw)
    return pixels16, pixels32


# ─── INF writers ─────────────────────────────────────────────────────────────

def write_inf_with_icons(out_path, display_name, pixels16, pixels32, ext_line):
    """Legacy format: name + 16x16 icon + 32x32 icon + ext line."""
    with open(out_path, "wb") as f:
        f.write(display_name.encode("ascii"))
        f.write(b"\n")
        f.write(bytes(pixels16))   # 256 bytes
        f.write(bytes(pixels32))   # 1024 bytes
        if ext_line:
            f.write(ext_line.encode("ascii"))
            f.write(b"\n")
    print(f"  wrote {out_path}  ({os.path.getsize(out_path)} bytes)")


def write_inf_iconless(out_path, display_name, ext_line):
    """New format: name + ext line only (icons come from .ico file)."""
    with open(out_path, "wb") as f:
        f.write(display_name.encode("ascii"))
        f.write(b"\n")
        if ext_line:
            f.write(ext_line.encode("ascii"))
            f.write(b"\n")
    print(f"  wrote {out_path}  ({os.path.getsize(out_path)} bytes)")


def deploy_ico(app_id, dest_dir):
    """Copy assets/apps/<app_id>.ico to dest_dir if it exists."""
    src = os.path.join(APP_ICONS, f"{app_id}.ico")
    if not os.path.isfile(src):
        return False
    dst = os.path.join(dest_dir, f"{app_id}.ico")
    shutil.copy2(src, dst)
    print(f"  copied {dst}  ({os.path.getsize(dst)} bytes)")
    return True


# ─── Main ────────────────────────────────────────────────────────────────────

def main():
    # Also deploy standalone .ico files that aren't tied to an app .inf
    # (desktop.ico, terminal.ico, navigator.ico)
    standalone_icos = ["desktop", "terminal", "navigator"]

    errors = []
    for (app_id, display_name, legacy_ico, ext_line) in APPS:
        print(f"\n[{app_id}] {display_name}")
        try:
            ico_src = os.path.join(APP_ICONS, f"{app_id}.ico")
            has_app_ico = os.path.isfile(ico_src)

            if has_app_ico:
                # New path: iconless .inf + deploy .ico file
                for dest_dir in (FOS_DIR, COMPILED_DIR):
                    inf_out = os.path.join(dest_dir, f"{app_id}.inf")
                    write_inf_iconless(inf_out, display_name, ext_line)
                    deploy_ico(app_id, dest_dir)
            else:
                # Legacy path: embedded icons in .inf
                if legacy_ico is not None:
                    ico_path = os.path.join(ICONS_DIR, legacy_ico)
                    pixels16 = read_ico_16(ico_path)
                    pixels32 = read_ico_32(ico_path)
                elif app_id == "digger":
                    pixels16, pixels32 = get_digger_icons()
                elif app_id == "zxspectrum":
                    pixels16, pixels32 = get_zxspectrum_icons()
                else:
                    raise ValueError(f"No icon source for '{app_id}'")

                for dest_dir in (FOS_DIR, COMPILED_DIR):
                    inf_out = os.path.join(dest_dir, f"{app_id}.inf")
                    write_inf_with_icons(inf_out, display_name,
                                         pixels16, pixels32, ext_line)

        except Exception as e:
            print(f"  ERROR: {e}", file=sys.stderr)
            errors.append(app_id)

    # Deploy standalone .ico files (not associated with .inf)
    print()
    for name in standalone_icos:
        src = os.path.join(APP_ICONS, f"{name}.ico")
        if os.path.isfile(src):
            for dest_dir in (FOS_DIR, COMPILED_DIR):
                dst = os.path.join(dest_dir, f"{name}.ico")
                shutil.copy2(src, dst)
                print(f"  deployed {dst}")

    print()
    if errors:
        print(f"Failed: {', '.join(errors)}", file=sys.stderr)
        sys.exit(1)
    else:
        print("All files regenerated successfully.")


if __name__ == "__main__":
    main()
