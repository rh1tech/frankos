/*
 * Originally from Murmulator OS 2 by DnCraptor
 * https://github.com/DnCraptor/murmulator-os2
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef __graphics_lines_h__
#define __graphics_lines_h__

#include <stdint.h>

typedef struct line {
   int8_t off; // left offset (-1 for center the line)
   const char* txt;
} line_t;

typedef struct lines {
   uint8_t sz;
   uint8_t toff; // top offset
   const line_t* plns;
} lines_t;

#endif // __graphics_lines_h__
