#pragma once

/*
 * Z80 ARM-ASM bisect switches.
 *
 * Goal: let us enable ARM-ASM opcode chunks incrementally and fall back to the
 * proven C interpreter (ExecZ80) for anything disabled.
 *
 * Notes:
 * - This header is included by `z80_arm.S` (preprocessed, .S).
 * - If `Z80_ARM_ENABLE` is 0, `z80_arm_exec()` just calls `ExecZ80()`.
 */

/* Master switch: 0 = always C core, 1 = run ARM-ASM dispatch where enabled. */
#define Z80_ARM_ENABLE 0

/* Chunk toggles (set to 1 to enable ASM handlers for these opcodes). */

/* 0x08: EX AF,AF' */
#define Z80_ARM_ENABLE_EX_AF 1

/* 0xC6/0xCE/0xD6/0xDE: ADD/ADC/SUB/SBC A,n */
#define Z80_ARM_ENABLE_IMM_ALU 1

/* 0x32/0x3A/0x36: LD (nn),A / LD A,(nn) / LD (HL),n */
#define Z80_ARM_ENABLE_MEM_IMM 1

/* 0xD9/0xEB/0xE3/0xF9: EXX / EX DE,HL / EX (SP),HL / LD SP,HL */
#define Z80_ARM_ENABLE_EX_MISC 1
