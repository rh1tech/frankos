/*
 * Z80 ARM Assembly Emulator for RP2350
 * Header file
 */
#ifndef Z80_ARM_H
#define Z80_ARM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Execute Z80 cycles using ARM assembly core
 * Input: cycles = number of cycles to execute
 * Returns: remaining cycles (may be negative)
 */
int z80_arm_exec(int cycles);

#ifdef __cplusplus
}
#endif

#endif /* Z80_ARM_H */
