/*
 * FRANK OS
 * Copyright (c) 2025 Mikhail Matveev <xtreme@rh1.tech>
 * https://rh1.tech
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef _SDCARD_H_
#define _SDCARD_H_

#include "board_config.h"

/* SD card SPI pin mapping — uses definitions from board_config.h */

#ifndef SDCARD_SPI_BUS
#define SDCARD_SPI_BUS spi0
#endif

#ifndef SDCARD_PIN_SPI0_CS
#define SDCARD_PIN_SPI0_CS     SDCARD_PIN_D3
#endif

#ifndef SDCARD_PIN_SPI0_SCK
#define SDCARD_PIN_SPI0_SCK    SDCARD_PIN_CLK
#endif

#ifndef SDCARD_PIN_SPI0_MOSI
#define SDCARD_PIN_SPI0_MOSI   SDCARD_PIN_CMD
#endif

#ifndef SDCARD_PIN_SPI0_MISO
#define SDCARD_PIN_SPI0_MISO   SDCARD_PIN_D0
#endif

#endif /* _SDCARD_H_ */
