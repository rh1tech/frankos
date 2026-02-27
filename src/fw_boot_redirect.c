/*
 * FRANK OS — Over-Mode Boot Redirect Stub
 * Copyright (c) 2025 Mikhail Matveev <xtreme@rh1.tech>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Placed in .boot_redirect section at a fixed high-flash address (0x103FE000)
 * that survives over-mode firmware flashing.  For .uf2 firmware that starts at
 * flash offset 0, load_firmware_psram() patches the firmware's Reset vector
 * (word[1] of sector 0) to point to _boot_redirect_entry below.
 *
 * On boot the RP2350 ROM processes the firmware's IMAGE_DEF, then jumps
 * to the patched Reset vector → _boot_redirect_entry:
 *   1. Space key held → erase ZERO_BLOCK, enter USB bootloader
 *   2. ZERO_BLOCK valid → jump to firmware's original Reset from ZERO_BLOCK
 *   3. Otherwise        → enter USB bootloader (safety fallback)
 *
 * For non-over-mode (.m1p2), the before_main() constructor in main.c
 * handles the redirect instead — this stub is not involved.
 */

#include <stdint.h>

#define FLASH_MAGIC_OVER  0x3836d91au
#define ZB_FLASH_OFFSET   ((16ul << 20) - (1ul << 20) - (4ul << 10))  /* 0xEFF000 */
#define ZB_XIP_ADDR       ((volatile uint32_t *)(0x10000000 + ZB_FLASH_OFFSET))

/* PS/2 keyboard GPIO pins (board_config.h values) */
#define PS2_CLK_PIN  2
#define PS2_DATA_PIN 3

/* GPIO register addresses */
#define IO_BANK0_BASE     0x40028000u
#define PADS_BANK0_BASE   0x40038000u
#define SIO_BASE          0xD0000000u
#define GPIO_IN_REG       (*(volatile uint32_t *)(SIO_BASE + 0x008))

/* RP2350 ROM function lookup */
#define ROM_TABLE_LOOKUP  (*(uint16_t *)0x18)
#define RT_FLAG_ARM_SEC   0x0004u
#define ROM_RESET_USB_BOOT  (0x55 | (0x42 << 8))   /* 'U','B' */
#define ROM_FLASH_OP        (0x46 | (0x4F << 8))   /* 'F','O' */
#define FLASH_OP_ERASE   0x00030002u

/* ---- Helper: ROM function lookup ---- */
__attribute__((section(".boot_redirect"), noinline))
static void *rom_lookup(uint32_t code) {
    typedef void *(*rom_table_lookup_fn)(uint32_t code, uint32_t flags);
    rom_table_lookup_fn lookup =
        (rom_table_lookup_fn)(uintptr_t)ROM_TABLE_LOOKUP;
    return lookup(code, RT_FLAG_ARM_SEC);
}

/* ---- Enter USB bootloader (never returns) ---- */
__attribute__((section(".boot_redirect"), noreturn, noinline))
static void enter_usb_bootloader(void) {
    typedef void __attribute__((noreturn)) (*usb_boot_fn)(uint32_t, uint32_t);
    usb_boot_fn fn = (usb_boot_fn)rom_lookup(ROM_RESET_USB_BOOT);
    fn(0, 0);
    __builtin_unreachable();
}

/* ---- Erase ZERO_BLOCK (clears firmware marker in flash) ---- */
__attribute__((section(".boot_redirect"), noinline))
static void erase_zero_block(void) {
    typedef int (*flash_op_fn)(uint32_t flags, uint32_t addr,
                               uint32_t size, uint8_t *buf);
    flash_op_fn fop = (flash_op_fn)rom_lookup(ROM_FLASH_OP);
    fop(FLASH_OP_ERASE, ZB_FLASH_OFFSET, 4096, (uint8_t *)0);
}

/* ---- Raw PS/2 keyboard: check if Space key is held ---- */
__attribute__((section(".boot_redirect"), noinline))
static int check_space_key(void) {
    /* Configure GPIO 2 (CLK) and GPIO 3 (DATA) as SIO inputs with pull-ups.
     * GPIO_CTRL registers: IO_BANK0 + 4 + n*8.
     * Pad registers: PADS_BANK0 + 4 + n*4. */
    *(volatile uint32_t *)(IO_BANK0_BASE + 4 + 2 * 8) = 5; /* SIO */
    *(volatile uint32_t *)(IO_BANK0_BASE + 4 + 3 * 8) = 5;

    /* Pad config: IE | PUE | SCHMITT = (1<<6)|(1<<3)|(1<<1) = 0x4A */
    uint32_t pad = (1u << 6) | (1u << 3) | (1u << 1);
    *(volatile uint32_t *)(PADS_BANK0_BASE + 4 + 2 * 4) = pad;
    *(volatile uint32_t *)(PADS_BANK0_BASE + 4 + 3 * 4) = pad;

    for (volatile int d = 0; d < 5000; d++) ;

    uint32_t clk_mask  = 1u << PS2_CLK_PIN;
    uint32_t data_mask = 1u << PS2_DATA_PIN;

    /* Try to receive PS/2 bytes for up to ~30 attempts.
     * If the user holds Space, the keyboard sends 0x29 repeatedly.
     * After power-on, the keyboard sends BAT (0xAA) first — we skip it. */
    for (int attempt = 0; attempt < 30; attempt++) {
        int timeout = 200000;
        while ((GPIO_IN_REG & clk_mask) && --timeout > 0) ;
        if (timeout <= 0) continue;

        timeout = 50000;
        while (!(GPIO_IN_REG & clk_mask) && --timeout > 0) ;
        if (timeout <= 0) continue;

        uint8_t data = 0;
        int ok = 1;
        for (int i = 0; i < 8 && ok; i++) {
            timeout = 50000;
            while ((GPIO_IN_REG & clk_mask) && --timeout > 0) ;
            if (timeout <= 0) { ok = 0; break; }
            if (GPIO_IN_REG & data_mask) data |= (1u << i);
            timeout = 50000;
            while (!(GPIO_IN_REG & clk_mask) && --timeout > 0) ;
            if (timeout <= 0) { ok = 0; break; }
        }
        if (!ok) continue;

        /* Skip parity bit */
        timeout = 50000;
        while ((GPIO_IN_REG & clk_mask) && --timeout > 0) ;
        timeout = 50000;
        while (!(GPIO_IN_REG & clk_mask) && --timeout > 0) ;

        /* Skip stop bit */
        timeout = 50000;
        while ((GPIO_IN_REG & clk_mask) && --timeout > 0) ;
        timeout = 50000;
        while (!(GPIO_IN_REG & clk_mask) && --timeout > 0) ;

        if (data == 0x29) return 1;   /* Space (PS/2 Set 2) */
    }
    return 0;
}

/*
 * Over-mode redirect entry point.
 * Called directly from the firmware's patched vector table Reset entry.
 * Externally visible so app.c can reference its address for the patch.
 *
 * VTOR stays at 0x10000000 — firmware's vectors (except patched Reset)
 * are already at offset 0 in flash.  This matches MOS2 over-mode behavior.
 */
__attribute__((section(".boot_redirect"), used))
void _boot_redirect_entry(void) {
    if (check_space_key()) {
        erase_zero_block();
        enter_usb_bootloader();
    }

    if (ZB_XIP_ADDR[1023] != FLASH_MAGIC_OVER) {
        enter_usb_bootloader();  /* safety fallback */
    }

    /* Jump to firmware's original Reset handler from ZERO_BLOCK.
     * ZERO_BLOCK[0] = firmware's initial SP
     * ZERO_BLOCK[1] = firmware's original Reset handler address */
    __asm volatile (
        "ldr r0, =%[zb_addr]  \n"
        "ldmia r0, {r0, r1}   \n"
        "msr msp, r0           \n"
        "bx r1                 \n"
        :: [zb_addr] "X" (0x10000000 + ZB_FLASH_OFFSET)
        : "r0", "r1", "memory"
    );
    __builtin_unreachable();
}
