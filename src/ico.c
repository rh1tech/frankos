/*
 * FRANK OS — Windows ICO file parser
 * Copyright (c) 2026 Mikhail Matveev <xtreme@rh1.tech>
 * https://rh1.tech
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ico.h"
#include <string.h>

/* CGA 16-color palette in RGB (used to match ICO palette entries) */
static const uint8_t CGA_RGB[16][3] = {
    {  0,   0,   0}, /*  0 Black       */
    {  0,   0, 170}, /*  1 Blue        */
    {  0, 170,   0}, /*  2 Green       */
    {  0, 170, 170}, /*  3 Cyan        */
    {170,   0,   0}, /*  4 Red         */
    {170,   0, 170}, /*  5 Magenta     */
    {170,  85,   0}, /*  6 Brown       */
    {170, 170, 170}, /*  7 Light Gray  */
    { 85,  85,  85}, /*  8 Dark Gray   */
    { 85,  85, 255}, /*  9 Light Blue  */
    { 85, 255,  85}, /* 10 Light Green */
    { 85, 255, 255}, /* 11 Light Cyan  */
    {255,  85,  85}, /* 12 Light Red   */
    {255,  85, 255}, /* 13 Light Mag   */
    {255, 255,  85}, /* 14 Yellow      */
    {255, 255, 255}, /* 15 White       */
};

/* Find the closest CGA color for an RGB value (squared Euclidean distance) */
static uint8_t closest_cga(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t best_dist = UINT32_MAX;
    uint8_t  best_idx  = 0;
    for (int i = 0; i < 16; i++) {
        int dr = (int)r - CGA_RGB[i][0];
        int dg = (int)g - CGA_RGB[i][1];
        int db = (int)b - CGA_RGB[i][2];
        uint32_t dist = (uint32_t)(dr*dr + dg*dg + db*db);
        if (dist < best_dist) {
            best_dist = dist;
            best_idx  = (uint8_t)i;
        }
    }
    return best_idx;
}

/*==========================================================================
 * ICO file structures (packed on-disk format)
 *=========================================================================*/

/* ICO header: 6 bytes */
typedef struct {
    uint16_t reserved;   /* must be 0 */
    uint16_t type;       /* 1 = icon */
    uint16_t count;      /* number of images */
} ico_header_t;

/* ICO directory entry: 16 bytes */
typedef struct {
    uint8_t  width;      /* 0 means 256 */
    uint8_t  height;     /* 0 means 256 */
    uint8_t  color_count;
    uint8_t  reserved;
    uint16_t planes;
    uint16_t bpp;
    uint32_t size;
    uint32_t offset;
} ico_dir_entry_t;

/* BMP info header: 40 bytes */
typedef struct {
    uint32_t hdr_size;
    int32_t  width;
    int32_t  height;     /* doubled: includes XOR + AND mask */
    uint16_t planes;
    uint16_t bpp;
    uint32_t compression;
    uint32_t img_size;
    int32_t  xppm;
    int32_t  yppm;
    uint32_t colors_used;
    uint32_t colors_important;
} bmp_info_header_t;

/*==========================================================================
 * Helpers — read little-endian values from byte buffer
 *=========================================================================*/

static inline uint16_t rd16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static inline uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static inline int32_t rdi32(const uint8_t *p) {
    return (int32_t)rd32(p);
}

/*==========================================================================
 * Find an ICO directory entry for a given target size
 *=========================================================================*/

/* Returns the file offset of the BMP data, or 0 if not found */
static uint32_t find_entry(const uint8_t *data, size_t len, int target_size) {
    if (len < 6) return 0;

    uint16_t type  = rd16(data + 2);
    uint16_t count = rd16(data + 4);
    if (type != 1) return 0;

    for (int i = 0; i < count; i++) {
        size_t dir_off = 6 + (size_t)i * 16;
        if (dir_off + 16 > len) break;

        int w = data[dir_off];
        if (w == 0) w = 256;
        int h = data[dir_off + 1];
        if (h == 0) h = 256;

        if (w == target_size && h == target_size) {
            uint32_t offset = rd32(data + dir_off + 12);
            return offset;
        }
    }
    return 0;
}

/*==========================================================================
 * Extract a 4bpp BMP image from ICO data at given offset
 *=========================================================================*/

static bool extract_4bpp(const uint8_t *data, size_t len,
                         uint32_t img_offset, int expected_size,
                         uint8_t *out) {
    /* Need at least 40 bytes for BMP info header */
    if (img_offset + 40 > len) return false;

    const uint8_t *hdr = data + img_offset;
    int32_t  bmp_w   = rdi32(hdr + 4);
    int32_t  bmp_h   = rdi32(hdr + 8);
    uint16_t bpp     = rd16(hdr + 14);
    uint32_t colors  = rd32(hdr + 32);

    int actual_h = bmp_h / 2;  /* BMP height is doubled (XOR + AND) */

    if (bpp != 4) return false;
    if (bmp_w != expected_size || actual_h != expected_size) return false;

    if (colors == 0) colors = 16;
    uint32_t hdr_size = rd32(hdr);

    uint32_t palette_off = img_offset + hdr_size;
    if (palette_off + colors * 4 > len) return false;

    /* Build palette → CGA remap table from actual ICO palette RGB values */
    uint8_t remap[16];
    for (uint32_t i = 0; i < colors && i < 16; i++) {
        uint32_t poff = palette_off + i * 4;
        uint8_t b = data[poff];
        uint8_t g = data[poff + 1];
        uint8_t r = data[poff + 2];
        remap[i] = closest_cga(r, g, b);
    }

    uint32_t xor_off = palette_off + colors * 4;

    int row_bytes  = (bmp_w * 4 + 7) / 8;          /* bytes per XOR row */
    int row_stride = (row_bytes + 3) & ~3;          /* padded to 4 bytes */
    uint32_t and_off = xor_off + (uint32_t)row_stride * actual_h;

    int and_row_bytes = (bmp_w + 7) / 8;
    int and_stride    = (and_row_bytes + 3) & ~3;

    /* Bounds check */
    if (and_off + (uint32_t)and_stride * actual_h > len) return false;

    for (int row = 0; row < actual_h; row++) {
        int bmp_row = actual_h - 1 - row;   /* BMP is bottom-up */
        const uint8_t *xor_row = data + xor_off + bmp_row * row_stride;
        const uint8_t *and_row = data + and_off + bmp_row * and_stride;

        for (int col = 0; col < bmp_w; col++) {
            /* Extract 4bpp palette index */
            uint8_t byte = xor_row[col / 2];
            uint8_t pal_idx = (col & 1) ? (byte & 0x0F) : (byte >> 4);

            /* Check AND mask (1 = transparent) */
            uint8_t and_byte = and_row[col / 8];
            uint8_t and_bit  = (and_byte >> (7 - (col & 7))) & 1;

            if (and_bit) {
                out[row * expected_size + col] = 0xFF;  /* transparent */
            } else {
                out[row * expected_size + col] = remap[pal_idx & 0x0F];
            }
        }
    }
    return true;
}

/*==========================================================================
 * Nearest-neighbour scaling
 *=========================================================================*/

static void nn_downscale_32_to_16(const uint8_t *src, uint8_t *dst) {
    for (int y = 0; y < 16; y++) {
        int sy = y * 32 / 16;
        for (int x = 0; x < 16; x++) {
            int sx = x * 32 / 16;
            dst[y * 16 + x] = src[sy * 32 + sx];
        }
    }
}

static void nn_upscale_16_to_32(const uint8_t *src, uint8_t *dst) {
    for (int y = 0; y < 32; y++) {
        int sy = y * 16 / 32;
        for (int x = 0; x < 32; x++) {
            int sx = x * 16 / 32;
            dst[y * 32 + x] = src[sy * 16 + sx];
        }
    }
}

/*==========================================================================
 * Public API
 *=========================================================================*/

bool ico_parse_16(const uint8_t *data, size_t len, uint8_t out[256]) {
    /* Try 16x16 first */
    uint32_t off = find_entry(data, len, 16);
    if (off && extract_4bpp(data, len, off, 16, out))
        return true;

    /* Fall back: extract 32x32 and downscale */
    off = find_entry(data, len, 32);
    if (off) {
        uint8_t tmp[1024];
        if (extract_4bpp(data, len, off, 32, tmp)) {
            nn_downscale_32_to_16(tmp, out);
            return true;
        }
    }
    return false;
}

bool ico_parse_32(const uint8_t *data, size_t len, uint8_t out[1024]) {
    /* Try 32x32 first */
    uint32_t off = find_entry(data, len, 32);
    if (off && extract_4bpp(data, len, off, 32, out))
        return true;

    /* Fall back: extract 16x16 and upscale */
    off = find_entry(data, len, 16);
    if (off) {
        uint8_t tmp[256];
        if (extract_4bpp(data, len, off, 16, tmp)) {
            nn_upscale_16_to_32(tmp, out);
            return true;
        }
    }
    return false;
}
