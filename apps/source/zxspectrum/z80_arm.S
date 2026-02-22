/*
 * Z80 ARM Assembly Emulator for RP2350
 * Step 4: add HALT (0x76), JP nn (0xC3), RET (0xC9)
 *  - Now: NOP, LD A,n, LD HL,nn, LD (HL),A, LD A,(HL), HALT, JP nn, RET
 *  - Others fall back to ExecZ80
 *  - Loop executes multiple instructions per call until cycles <= 0
 */

.syntax unified
.cpu cortex-m33
.thumb

.section .text

#include "z80_arm_config.h"

/*
 * All mutable state is in a heap-allocated app_globals_t struct whose
 * pointer lives in register r9 (set by main(), preserved by FreeRTOS
 * context switch).  Z80 is the FIRST member of app_globals_t, so r9
 * also serves as the Z80 struct pointer.
 */

.extern RdZ80
.extern ExecZ80
.extern WrZ80

/* Z80 structure offsets */
.equ CPU_AF,     0
.equ CPU_BC,     2
.equ CPU_DE,     4
.equ CPU_HL,     6
.equ CPU_IX,     8
.equ CPU_IY,     10
.equ CPU_PC,     12
.equ CPU_SP,     14
.equ CPU_AF1,    16
.equ CPU_BC1,    18
.equ CPU_DE1,    20
.equ CPU_HL1,    22
.equ CPU_IFF,    24
.equ CPU_I,      25
.equ CPU_R,      26
.equ CPU_IPERIOD, 28
.equ CPU_ICOUNT, 32
.equ CPU_IBACKUP, 40
.equ CPU_IREQUEST, 44

/* IFF flags */
.equ IFF_HALT, 0x80

/* Flag bits */
.equ F_S, 0x80
.equ F_Z, 0x40
.equ F_Y, 0x20
.equ F_H, 0x10
.equ F_X, 0x08
.equ F_V, 0x04
.equ F_N, 0x02
.equ F_C, 0x01

.global z80_arm_exec
.type z80_arm_exec, %function
.thumb_func
z80_arm_exec:
#if !Z80_ARM_ENABLE
    /* Wrapper mode: always use C interpreter.
     * r9 = app_globals_t *, Z80 is first member, so r9 = Z80 *. */
    push    {r4, lr}
    mov     r1, r0              /* r1 = cycles */
    mov     r0, r9              /* r0 = Z80 * (r9 = app_globals_t *) */
    bl      ExecZ80
    pop     {r4, pc}
#else
    push    {r4-r7, lr}

    /* r0 = cycles */
    mov     r4, r0             /* r4 = cycles remaining */

    /* r9 = app_globals_t *, Z80 is first member, so r9 = Z80 * */
    mov     r6, r9             /* r6 = Z80 struct pointer */
    ldrh    r7, [r6, #CPU_PC]
    uxth    r7, r7

.Lloop:
    cmp     r4, #0
    ble     .Lreturn

    /* Fetch opcode */
    mov     r0, r7
    bl      RdZ80
    /* r0 = opcode */

    /* Dispatch */
    cmp     r0, #0x00
    beq     op_00
    cmp     r0, #0x01
    beq     op_01_ld_bc_nn
    cmp     r0, #0x03
    beq     op_03_inc_bc
    cmp     r0, #0x04
    beq     op_04_inc_b
    cmp     r0, #0x05
    beq     op_05_dec_b
    cmp     r0, #0x0B
    beq     op_0b_dec_bc
    cmp     r0, #0x0C
    beq     op_0c_inc_c
    cmp     r0, #0x0D
    beq     op_0d_dec_c
    cmp     r0, #0x09
    beq     op_09_add_hl_bc
    cmp     r0, #0x11
    beq     op_11_ld_de_nn
    cmp     r0, #0x14
    beq     op_14_inc_d
    cmp     r0, #0x15
    beq     op_15_dec_d
    cmp     r0, #0x1C
    beq     op_1c_inc_e
    cmp     r0, #0x1D
    beq     op_1d_dec_e
    cmp     r0, #0x24
    beq     op_24_inc_h
    cmp     r0, #0x25
    beq     op_25_dec_h
    cmp     r0, #0x2C
    beq     op_2c_inc_l
    cmp     r0, #0x2D
    beq     op_2d_dec_l
    cmp     r0, #0x34
    beq     op_34_inc_hl
    cmp     r0, #0x35
    beq     op_35_dec_hl
    cmp     r0, #0x3C
    beq     op_3c_inc_a
    cmp     r0, #0x3D
    beq     op_3d_dec_a
    cmp     r0, #0x3E
    beq     op_3e_ld_a_n
#if Z80_ARM_ENABLE_MEM_IMM
    cmp     r0, #0x32
    beq     op_32_ld_nn_a
    cmp     r0, #0x3A
    beq     op_3a_ld_a_nn
    cmp     r0, #0x36
    beq     op_36_ld_hl_n
#endif
    cmp     r0, #0x22
    beq     op_22_ld_nn_hl
#if Z80_ARM_ENABLE_EX_AF
    cmp     r0, #0x08
    beq     op_08_ex_af_af1
#endif
    cmp     r0, #0x13
    beq     op_13_inc_de
    cmp     r0, #0x1B
    beq     op_1b_dec_de
    cmp     r0, #0x19
    beq     op_19_add_hl_de
    cmp     r0, #0x23
    beq     op_23_inc_hl
    cmp     r0, #0x2A
    beq     op_2a_ld_hl_nn
    cmp     r0, #0x2B
    beq     op_2b_dec_hl
    cmp     r0, #0x29
    beq     op_29_add_hl_hl
    cmp     r0, #0x31
    beq     op_31_ld_sp_nn
    cmp     r0, #0x33
    beq     op_33_inc_sp
    cmp     r0, #0x3B
    beq     op_3b_dec_sp
    cmp     r0, #0x39
    beq     op_39_add_hl_sp
    cmp     r0, #0xC1
    beq     op_c1_pop_bc
    cmp     r0, #0xD1
    beq     op_d1_pop_de
    cmp     r0, #0xE1
    beq     op_e1_pop_hl
    cmp     r0, #0xF1
    beq     op_f1_pop_af
    cmp     r0, #0xC5
    beq     op_c5_push_bc
    cmp     r0, #0xD5
    beq     op_d5_push_de
    cmp     r0, #0xE5
    beq     op_e5_push_hl
    cmp     r0, #0xF5
    beq     op_f5_push_af
    cmp     r0, #0x40
    beq     op_40_ld_b_b
    cmp     r0, #0x41
    beq     op_41_ld_b_c
    cmp     r0, #0x42
    beq     op_42_ld_b_d
    cmp     r0, #0x43
    beq     op_43_ld_b_e
    cmp     r0, #0x44
    beq     op_44_ld_b_h
    cmp     r0, #0x45
    beq     op_45_ld_b_l
    cmp     r0, #0x47
    beq     op_47_ld_b_a
    cmp     r0, #0x48
    beq     op_48_ld_c_b
    cmp     r0, #0x49
    beq     op_49_ld_c_c
    cmp     r0, #0x4A
    beq     op_4a_ld_c_d
    cmp     r0, #0x4B
    beq     op_4b_ld_c_e
    cmp     r0, #0x4C
    beq     op_4c_ld_c_h
    cmp     r0, #0x4D
    beq     op_4d_ld_c_l
    cmp     r0, #0x4F
    beq     op_4f_ld_c_a
    cmp     r0, #0x50
    beq     op_50_ld_d_b
    cmp     r0, #0x51
    beq     op_51_ld_d_c
    cmp     r0, #0x52
    beq     op_52_ld_d_d
    cmp     r0, #0x53
    beq     op_53_ld_d_e
    cmp     r0, #0x54
    beq     op_54_ld_d_h
    cmp     r0, #0x55
    beq     op_55_ld_d_l
    cmp     r0, #0x57
    beq     op_57_ld_d_a
    cmp     r0, #0x58
    beq     op_58_ld_e_b
    cmp     r0, #0x59
    beq     op_59_ld_e_c
    cmp     r0, #0x5A
    beq     op_5a_ld_e_d
    cmp     r0, #0x5B
    beq     op_5b_ld_e_e
    cmp     r0, #0x5C
    beq     op_5c_ld_e_h
    cmp     r0, #0x5D
    beq     op_5d_ld_e_l
    cmp     r0, #0x5F
    beq     op_5f_ld_e_a
    cmp     r0, #0x60
    beq     op_60_ld_h_b
    cmp     r0, #0x61
    beq     op_61_ld_h_c
    cmp     r0, #0x62
    beq     op_62_ld_h_d
    cmp     r0, #0x63
    beq     op_63_ld_h_e
    cmp     r0, #0x64
    beq     op_64_ld_h_h
    cmp     r0, #0x65
    beq     op_65_ld_h_l
    cmp     r0, #0x67
    beq     op_67_ld_h_a
    cmp     r0, #0x68
    beq     op_68_ld_l_b
    cmp     r0, #0x69
    beq     op_69_ld_l_c
    cmp     r0, #0x6A
    beq     op_6a_ld_l_d
    cmp     r0, #0x6B
    beq     op_6b_ld_l_e
    cmp     r0, #0x6C
    beq     op_6c_ld_l_h
    cmp     r0, #0x6D
    beq     op_6d_ld_l_l
    cmp     r0, #0x6F
    beq     op_6f_ld_l_a
    cmp     r0, #0x21
    beq     op_21_ld_hl_nn
    cmp     r0, #0x06
    beq     op_06_ld_b_n
    cmp     r0, #0x0E
    beq     op_0e_ld_c_n
    cmp     r0, #0x16
    beq     op_16_ld_d_n
    cmp     r0, #0x1E
    beq     op_1e_ld_e_n
    cmp     r0, #0x26
    beq     op_26_ld_h_n
    cmp     r0, #0x2E
    beq     op_2e_ld_l_n
    cmp     r0, #0x46
    beq     op_46_ld_b_hl
    cmp     r0, #0x4E
    beq     op_4e_ld_c_hl
    cmp     r0, #0x56
    beq     op_56_ld_d_hl
    cmp     r0, #0x5E
    beq     op_5e_ld_e_hl
    cmp     r0, #0x66
    beq     op_66_ld_h_hl
    cmp     r0, #0x6E
    beq     op_6e_ld_l_hl
    cmp     r0, #0x18
    beq     op_18_jr
    cmp     r0, #0x20
    beq     op_20_jr_nz
    cmp     r0, #0x28
    beq     op_28_jr_z
    cmp     r0, #0x30
    beq     op_30_jr_nc
    cmp     r0, #0x38
    beq     op_38_jr_c
    cmp     r0, #0xC2
    beq     op_c2_jp_nz
    cmp     r0, #0xCA
    beq     op_ca_jp_z
    cmp     r0, #0xD2
    beq     op_d2_jp_nc
    cmp     r0, #0xDA
    beq     op_da_jp_c
    cmp     r0, #0xE9
    beq     op_e9_jp_hl
#if Z80_ARM_ENABLE_EX_MISC
    cmp     r0, #0xEB
    beq     op_eb_ex_de_hl
    cmp     r0, #0xD9
    beq     op_d9_exx
    cmp     r0, #0xE3
    beq     op_e3_ex_sp_hl
    cmp     r0, #0xF9
    beq     op_f9_ld_sp_hl
#endif
    cmp     r0, #0x76
    beq     .Lfallback        /* HALT: use C core for now */
    cmp     r0, #0xC3
    beq     op_c3_jp
    cmp     r0, #0xC9
    beq     op_c9_ret
    cmp     r0, #0xCD
    beq     op_cd_call
    cmp     r0, #0xC7
    beq     op_c7_rst_00
    cmp     r0, #0xCF
    beq     op_cf_rst_08
    cmp     r0, #0xD7
    beq     op_d7_rst_10
    cmp     r0, #0xDF
    beq     op_df_rst_18
    cmp     r0, #0xE7
    beq     op_e7_rst_20
    cmp     r0, #0xEF
    beq     op_ef_rst_28
    cmp     r0, #0xF7
    beq     op_f7_rst_30
    cmp     r0, #0xFF
    beq     op_ff_rst_38
    cmp     r0, #0x78
    beq     op_78_ld_a_b
    cmp     r0, #0x79
    beq     op_79_ld_a_c
    cmp     r0, #0x7A
    beq     op_7a_ld_a_d
    cmp     r0, #0x7B
    beq     op_7b_ld_a_e
    cmp     r0, #0x7C
    beq     op_7c_ld_a_h
    cmp     r0, #0x7D
    beq     op_7d_ld_a_l
    cmp     r0, #0x77
    beq     op_77_ld_hl_a
    cmp     r0, #0x70
    beq     op_70_ld_hl_b
    cmp     r0, #0x71
    beq     op_71_ld_hl_c
    cmp     r0, #0x72
    beq     op_72_ld_hl_d
    cmp     r0, #0x73
    beq     op_73_ld_hl_e
    cmp     r0, #0x74
    beq     op_74_ld_hl_h
    cmp     r0, #0x75
    beq     op_75_ld_hl_l
    cmp     r0, #0x7E
    beq     op_7e_ld_a_hl
    cmp     r0, #0x7F
    beq     op_7f_ld_a_a
    cmp     r0, #0x80
    beq     op_80_add_a_b
    cmp     r0, #0x81
    beq     op_81_add_a_c
    cmp     r0, #0x82
    beq     op_82_add_a_d
    cmp     r0, #0x83
    beq     op_83_add_a_e
    cmp     r0, #0x84
    beq     op_84_add_a_h
    cmp     r0, #0x85
    beq     op_85_add_a_l
    cmp     r0, #0x86
    beq     op_86_add_a_hl
    cmp     r0, #0x87
    beq     op_87_add_a_a
    cmp     r0, #0x90
    beq     op_90_sub_b
    cmp     r0, #0x91
    beq     op_91_sub_c
    cmp     r0, #0x92
    beq     op_92_sub_d
    cmp     r0, #0x93
    beq     op_93_sub_e
    cmp     r0, #0x94
    beq     op_94_sub_h
    cmp     r0, #0x95
    beq     op_95_sub_l

#if Z80_ARM_ENABLE_IMM_ALU
    cmp     r0, #0xC6
    beq     op_c6_add_a_n
    cmp     r0, #0xCE
    beq     op_ce_adc_a_n
    cmp     r0, #0xD6
    beq     op_d6_sub_n
    cmp     r0, #0xDE
    beq     op_de_sbc_a_n
#endif
    cmp     r0, #0x96
    beq     op_96_sub_hl
    cmp     r0, #0x97
    beq     op_97_sub_a
    cmp     r0, #0x88
    beq     op_88_adc_a_b
    cmp     r0, #0x89
    beq     op_89_adc_a_c
    cmp     r0, #0x8A
    beq     op_8a_adc_a_d
    cmp     r0, #0x8B
    beq     op_8b_adc_a_e
    cmp     r0, #0x8C
    beq     op_8c_adc_a_h
    cmp     r0, #0x8D
    beq     op_8d_adc_a_l
    cmp     r0, #0x8E
    beq     op_8e_adc_a_hl
    cmp     r0, #0x8F
    beq     op_8f_adc_a_a
    cmp     r0, #0x98
    beq     op_98_sbc_a_b
    cmp     r0, #0x99
    beq     op_99_sbc_a_c
    cmp     r0, #0x9A
    beq     op_9a_sbc_a_d
    cmp     r0, #0x9B
    beq     op_9b_sbc_a_e
    cmp     r0, #0x9C
    beq     op_9c_sbc_a_h
    cmp     r0, #0x9D
    beq     op_9d_sbc_a_l
    cmp     r0, #0x9E
    beq     op_9e_sbc_a_hl
    cmp     r0, #0x9F
    beq     op_9f_sbc_a_a
    cmp     r0, #0x07
    beq     op_07_rlca
    cmp     r0, #0x0F
    beq     op_0f_rrca
    cmp     r0, #0x17
    beq     op_17_rla
    cmp     r0, #0x1F
    beq     op_1f_rra
    cmp     r0, #0x27
    beq     op_27_daa
    cmp     r0, #0x2F
    beq     op_2f_cpl
    cmp     r0, #0x37
    beq     op_37_scf
    cmp     r0, #0x3F
    beq     op_3f_ccf
    cmp     r0, #0xA0
    beq     op_a0_and_b
    cmp     r0, #0xA1
    beq     op_a1_and_c
    cmp     r0, #0xA2
    beq     op_a2_and_d
    cmp     r0, #0xA3
    beq     op_a3_and_e
    cmp     r0, #0xA4
    beq     op_a4_and_h
    cmp     r0, #0xA5
    beq     op_a5_and_l
    cmp     r0, #0xA6
    beq     op_a6_and_hl
    cmp     r0, #0xA7
    beq     op_a7_and_a
    cmp     r0, #0xA8
    beq     op_a8_xor_b
    cmp     r0, #0xA9
    beq     op_a9_xor_c
    cmp     r0, #0xAA
    beq     op_aa_xor_d
    cmp     r0, #0xAB
    beq     op_ab_xor_e
    cmp     r0, #0xAC
    beq     op_ac_xor_h
    cmp     r0, #0xAD
    beq     op_ad_xor_l
    cmp     r0, #0xAE
    beq     op_ae_xor_hl
    cmp     r0, #0xAF
    beq     op_af_xor_a
    cmp     r0, #0xB0
    beq     op_b0_or_b
    cmp     r0, #0xB1
    beq     op_b1_or_c
    cmp     r0, #0xB2
    beq     op_b2_or_d
    cmp     r0, #0xB3
    beq     op_b3_or_e
    cmp     r0, #0xB4
    beq     op_b4_or_h
    cmp     r0, #0xB5
    beq     op_b5_or_l
    cmp     r0, #0xB6
    beq     op_b6_or_hl
    cmp     r0, #0xB7
    beq     op_b7_or_a
    cmp     r0, #0xB8
    beq     op_b8_cp_b
    cmp     r0, #0xB9
    beq     op_b9_cp_c
    cmp     r0, #0xBA
    beq     op_ba_cp_d
    cmp     r0, #0xBB
    beq     op_bb_cp_e
    cmp     r0, #0xBC
    beq     op_bc_cp_h
    cmp     r0, #0xBD
    beq     op_bd_cp_l
    cmp     r0, #0xBE
    beq     op_be_cp_hl
    cmp     r0, #0xBF
    beq     op_bf_cp_a
    cmp     r0, #0xC6
    beq     op_c6_add_a_n
    cmp     r0, #0xCE
    beq     op_ce_adc_a_n
    cmp     r0, #0xD6
    beq     op_d6_sub_n
    cmp     r0, #0xDE
    beq     op_de_sbc_a_n
    cmp     r0, #0xE6
    beq     op_e6_and_n
    cmp     r0, #0xEE
    beq     op_ee_xor_n
    cmp     r0, #0xF6
    beq     op_f6_or_n
    cmp     r0, #0xFE
    beq     op_fe_cp_n
    b       .Lfallback

/* 0x00: NOP (4 cycles) */
op_00:
    add     r7, r7, #1         /* PC +=1 */
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x01: LD BC,nn (10 cycles) */
op_01_ld_bc_nn:
    add     r0, r7, #1         /* low */
    uxth    r0, r0
    bl      RdZ80
    uxtb    r5, r0             /* low byte */
    add     r0, r7, #2         /* high */
    uxth    r0, r0
    bl      RdZ80
    uxtb    r0, r0
    orr     r5, r5, r0, lsl #8
    uxth    r5, r5
    strh    r5, [r6, #CPU_BC]
    add     r7, r7, #3
    subs    r4, r4, #10
    bgt     .Lloop
    b       .Lreturn

/* 0x11: LD DE,nn (10 cycles) */
op_11_ld_de_nn:
    add     r0, r7, #1
    uxth    r0, r0
    bl      RdZ80
    uxtb    r5, r0
    add     r0, r7, #2
    uxth    r0, r0
    bl      RdZ80
    uxtb    r0, r0
    orr     r5, r5, r0, lsl #8
    uxth    r5, r5
    strh    r5, [r6, #CPU_DE]
    add     r7, r7, #3
    subs    r4, r4, #10
    bgt     .Lloop
    b       .Lreturn

/* 0x31: LD SP,nn (10 cycles) */
op_31_ld_sp_nn:
    add     r0, r7, #1
    uxth    r0, r0
    bl      RdZ80
    uxtb    r5, r0
    add     r0, r7, #2
    uxth    r0, r0
    bl      RdZ80
    uxtb    r0, r0
    orr     r5, r5, r0, lsl #8
    uxth    r5, r5
    strh    r5, [r6, #CPU_SP]
    add     r7, r7, #3
    subs    r4, r4, #10
    bgt     .Lloop
    b       .Lreturn

/* 0x09: ADD HL,BC (11 cycles) */
op_09_add_hl_bc:
    ldrh    r5, [r6, #CPU_HL]
    ldrh    r0, [r6, #CPU_BC]
    adds    r2, r5, r0         /* set carry */
    movs    r1, #0
    adc     r1, r1, #0         /* r1 = carry (0/1) */
    uxth    r2, r2
    /* halfcarry: (HL ^ rr ^ result) & 0x1000 */
    eor     r0, r5, r0
    eor     r0, r0, r2
    tst     r0, #0x1000
    /* flags: preserve all but H,N,C */
    ldrh    r3, [r6, #CPU_AF]
    and     r12, r3, #0xFF00
    uxtb    r3, r3
    bic     r3, r3, #(F_H | F_N | F_C)
    it      ne
    orrne   r3, r3, #F_H
    cmp     r1, #0
    it      ne
    orrne   r3, r3, #F_C
    orr     r3, r12, r3
    strh    r3, [r6, #CPU_AF]
    strh    r2, [r6, #CPU_HL]
    add     r7, r7, #1
    subs    r4, r4, #11
    bgt     .Lloop
    b       .Lreturn

/* 0x19: ADD HL,DE (11 cycles) */
op_19_add_hl_de:
    ldrh    r5, [r6, #CPU_HL]
    ldrh    r0, [r6, #CPU_DE]
    adds    r2, r5, r0
    movs    r1, #0
    adc     r1, r1, #0
    uxth    r2, r2
    eor     r0, r5, r0
    eor     r0, r0, r2
    tst     r0, #0x1000
    ldrh    r3, [r6, #CPU_AF]
    and     r12, r3, #0xFF00
    uxtb    r3, r3
    bic     r3, r3, #(F_H | F_N | F_C)
    it      ne
    orrne   r3, r3, #F_H
    cmp     r1, #0
    it      ne
    orrne   r3, r3, #F_C
    orr     r3, r12, r3
    strh    r3, [r6, #CPU_AF]
    strh    r2, [r6, #CPU_HL]
    add     r7, r7, #1
    subs    r4, r4, #11
    bgt     .Lloop
    b       .Lreturn

/* 0x29: ADD HL,HL (11 cycles) */
op_29_add_hl_hl:
    ldrh    r5, [r6, #CPU_HL]
    mov     r0, r5
    adds    r2, r5, r0
    movs    r1, #0
    adc     r1, r1, #0
    uxth    r2, r2
    eor     r0, r5, r0
    eor     r0, r0, r2
    tst     r0, #0x1000
    ldrh    r3, [r6, #CPU_AF]
    and     r12, r3, #0xFF00
    uxtb    r3, r3
    bic     r3, r3, #(F_H | F_N | F_C)
    it      ne
    orrne   r3, r3, #F_H
    cmp     r1, #0
    it      ne
    orrne   r3, r3, #F_C
    orr     r3, r12, r3
    strh    r3, [r6, #CPU_AF]
    strh    r2, [r6, #CPU_HL]
    add     r7, r7, #1
    subs    r4, r4, #11
    bgt     .Lloop
    b       .Lreturn

/* 0x39: ADD HL,SP (11 cycles) */
op_39_add_hl_sp:
    ldrh    r5, [r6, #CPU_HL]
    ldrh    r0, [r6, #CPU_SP]
    adds    r2, r5, r0
    movs    r1, #0
    adc     r1, r1, #0
    uxth    r2, r2
    eor     r0, r5, r0
    eor     r0, r0, r2
    tst     r0, #0x1000
    ldrh    r3, [r6, #CPU_AF]
    and     r12, r3, #0xFF00
    uxtb    r3, r3
    bic     r3, r3, #(F_H | F_N | F_C)
    it      ne
    orrne   r3, r3, #F_H
    cmp     r1, #0
    it      ne
    orrne   r3, r3, #F_C
    orr     r3, r12, r3
    strh    r3, [r6, #CPU_AF]
    strh    r2, [r6, #CPU_HL]
    add     r7, r7, #1
    subs    r4, r4, #11
    bgt     .Lloop
    b       .Lreturn

/* 0x22: LD (nn),HL (16 cycles) */
op_22_ld_nn_hl:
    push    {r8}
    /* fetch nn */
    add     r0, r7, #1
    uxth    r0, r0
    bl      RdZ80
    uxtb    r5, r0             /* low(addr) */
    add     r0, r7, #2
    uxth    r0, r0
    bl      RdZ80
    uxtb    r0, r0
    orr     r8, r5, r0, lsl #8
    uxth    r8, r8

    /* write low(HL) */
    ldrh    r5, [r6, #CPU_HL]
    uxtb    r1, r5             /* L */
    mov     r0, r8
    bl      WrZ80

    /* write high(HL) to nn+1 */
    ldrh    r5, [r6, #CPU_HL]
    lsr     r1, r5, #8         /* H */
    uxtb    r1, r1
    add     r0, r8, #1
    uxth    r0, r0
    bl      WrZ80

    add     r7, r7, #3
    subs    r4, r4, #16
    pop     {r8}
    bgt     .Lloop
    b       .Lreturn

/* 0x2A: LD HL,(nn) (16 cycles) */
op_2a_ld_hl_nn:
    push    {r8}
    /* fetch nn */
    add     r0, r7, #1
    uxth    r0, r0
    bl      RdZ80
    uxtb    r5, r0
    add     r0, r7, #2
    uxth    r0, r0
    bl      RdZ80
    uxtb    r0, r0
    orr     r8, r5, r0, lsl #8
    uxth    r8, r8

    /* read low */
    mov     r0, r8
    bl      RdZ80
    uxtb    r5, r0             /* low byte */

    /* read high */
    add     r0, r8, #1
    uxth    r0, r0
    bl      RdZ80
    uxtb    r0, r0
    orr     r5, r5, r0, lsl #8
    uxth    r5, r5
    strh    r5, [r6, #CPU_HL]

    add     r7, r7, #3
    subs    r4, r4, #16
    pop     {r8}
    bgt     .Lloop
    b       .Lreturn

/* 0x03: INC BC (6 cycles) */
op_03_inc_bc:
    ldrh    r5, [r6, #CPU_BC]
    add     r5, r5, #1
    uxth    r5, r5
    strh    r5, [r6, #CPU_BC]
    add     r7, r7, #1
    subs    r4, r4, #6
    bgt     .Lloop
    b       .Lreturn

/* 0x0B: DEC BC (6 cycles) */
op_0b_dec_bc:
    ldrh    r5, [r6, #CPU_BC]
    subs    r5, r5, #1
    uxth    r5, r5
    strh    r5, [r6, #CPU_BC]
    add     r7, r7, #1
    subs    r4, r4, #6
    bgt     .Lloop
    b       .Lreturn

/* 0x04: INC B (4 cycles) */
op_04_inc_b:
    ldrh    r5, [r6, #CPU_BC]
    lsrs    r0, r5, #8
    uxtb    r0, r0
    adds    r0, r0, #1
    uxtb    r0, r0
    bfi     r5, r0, #8, #8
    strh    r5, [r6, #CPU_BC]
    ldrh    r2, [r6, #CPU_AF]
    uxtb    r3, r2
    ands    r3, r3, #F_C
    cmp     r0, #0
    it      eq
    orreq   r3, r3, #F_Z
    tst     r0, #0x80
    it      ne
    orrne   r3, r3, #F_S
    cmp     r0, #0x80
    it      eq
    orreq   r3, r3, #F_V
    tst     r0, #0x0F
    it      eq
    orreq   r3, r3, #F_H
    and     r2, r2, #0xFF00
    orr     r2, r2, r3
    strh    r2, [r6, #CPU_AF]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x05: DEC B (4 cycles) */
op_05_dec_b:
    ldrh    r5, [r6, #CPU_BC]
    lsrs    r0, r5, #8
    uxtb    r0, r0
    subs    r0, r0, #1
    uxtb    r0, r0
    bfi     r5, r0, #8, #8
    strh    r5, [r6, #CPU_BC]
    ldrh    r2, [r6, #CPU_AF]
    uxtb    r3, r2
    ands    r3, r3, #F_C
    orr     r3, r3, #F_N
    cmp     r0, #0
    it      eq
    orreq   r3, r3, #F_Z
    tst     r0, #0x80
    it      ne
    orrne   r3, r3, #F_S
    cmp     r0, #0x7F
    it      eq
    orreq   r3, r3, #F_V
    ands    r1, r0, #0x0F
    cmp     r1, #0x0F
    it      eq
    orreq   r3, r3, #F_H
    and     r2, r2, #0xFF00
    orr     r2, r2, r3
    strh    r2, [r6, #CPU_AF]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x0C: INC C (4 cycles) */
op_0c_inc_c:
    ldrh    r5, [r6, #CPU_BC]
    uxtb    r0, r5
    adds    r0, r0, #1
    uxtb    r0, r0
    bfi     r5, r0, #0, #8
    strh    r5, [r6, #CPU_BC]
    ldrh    r2, [r6, #CPU_AF]
    uxtb    r3, r2
    ands    r3, r3, #F_C
    cmp     r0, #0
    it      eq
    orreq   r3, r3, #F_Z
    tst     r0, #0x80
    it      ne
    orrne   r3, r3, #F_S
    cmp     r0, #0x80
    it      eq
    orreq   r3, r3, #F_V
    tst     r0, #0x0F
    it      eq
    orreq   r3, r3, #F_H
    and     r2, r2, #0xFF00
    orr     r2, r2, r3
    strh    r2, [r6, #CPU_AF]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x0D: DEC C (4 cycles) */
op_0d_dec_c:
    ldrh    r5, [r6, #CPU_BC]
    uxtb    r0, r5
    subs    r0, r0, #1
    uxtb    r0, r0
    bfi     r5, r0, #0, #8
    strh    r5, [r6, #CPU_BC]
    ldrh    r2, [r6, #CPU_AF]
    uxtb    r3, r2
    ands    r3, r3, #F_C
    orr     r3, r3, #F_N
    cmp     r0, #0
    it      eq
    orreq   r3, r3, #F_Z
    tst     r0, #0x80
    it      ne
    orrne   r3, r3, #F_S
    cmp     r0, #0x7F
    it      eq
    orreq   r3, r3, #F_V
    ands    r1, r0, #0x0F
    cmp     r1, #0x0F
    it      eq
    orreq   r3, r3, #F_H
    and     r2, r2, #0xFF00
    orr     r2, r2, r3
    strh    r2, [r6, #CPU_AF]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x14: INC D (4 cycles) */
op_14_inc_d:
    ldrh    r5, [r6, #CPU_DE]
    lsrs    r0, r5, #8
    uxtb    r0, r0
    adds    r0, r0, #1
    uxtb    r0, r0
    bfi     r5, r0, #8, #8
    strh    r5, [r6, #CPU_DE]
    ldrh    r2, [r6, #CPU_AF]
    uxtb    r3, r2
    ands    r3, r3, #F_C
    cmp     r0, #0
    it      eq
    orreq   r3, r3, #F_Z
    tst     r0, #0x80
    it      ne
    orrne   r3, r3, #F_S
    cmp     r0, #0x80
    it      eq
    orreq   r3, r3, #F_V
    tst     r0, #0x0F
    it      eq
    orreq   r3, r3, #F_H
    and     r2, r2, #0xFF00
    orr     r2, r2, r3
    strh    r2, [r6, #CPU_AF]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x15: DEC D (4 cycles) */
op_15_dec_d:
    ldrh    r5, [r6, #CPU_DE]
    lsrs    r0, r5, #8
    uxtb    r0, r0
    subs    r0, r0, #1
    uxtb    r0, r0
    bfi     r5, r0, #8, #8
    strh    r5, [r6, #CPU_DE]
    ldrh    r2, [r6, #CPU_AF]
    uxtb    r3, r2
    ands    r3, r3, #F_C
    orr     r3, r3, #F_N
    cmp     r0, #0
    it      eq
    orreq   r3, r3, #F_Z
    tst     r0, #0x80
    it      ne
    orrne   r3, r3, #F_S
    cmp     r0, #0x7F
    it      eq
    orreq   r3, r3, #F_V
    ands    r1, r0, #0x0F
    cmp     r1, #0x0F
    it      eq
    orreq   r3, r3, #F_H
    and     r2, r2, #0xFF00
    orr     r2, r2, r3
    strh    r2, [r6, #CPU_AF]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x1C: INC E (4 cycles) */
op_1c_inc_e:
    ldrh    r5, [r6, #CPU_DE]
    uxtb    r0, r5
    adds    r0, r0, #1
    uxtb    r0, r0
    bfi     r5, r0, #0, #8
    strh    r5, [r6, #CPU_DE]
    ldrh    r2, [r6, #CPU_AF]
    uxtb    r3, r2
    ands    r3, r3, #F_C
    cmp     r0, #0
    it      eq
    orreq   r3, r3, #F_Z
    tst     r0, #0x80
    it      ne
    orrne   r3, r3, #F_S
    cmp     r0, #0x80
    it      eq
    orreq   r3, r3, #F_V
    tst     r0, #0x0F
    it      eq
    orreq   r3, r3, #F_H
    and     r2, r2, #0xFF00
    orr     r2, r2, r3
    strh    r2, [r6, #CPU_AF]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x1D: DEC E (4 cycles) */
op_1d_dec_e:
    ldrh    r5, [r6, #CPU_DE]
    uxtb    r0, r5
    subs    r0, r0, #1
    uxtb    r0, r0
    bfi     r5, r0, #0, #8
    strh    r5, [r6, #CPU_DE]
    ldrh    r2, [r6, #CPU_AF]
    uxtb    r3, r2
    ands    r3, r3, #F_C
    orr     r3, r3, #F_N
    cmp     r0, #0
    it      eq
    orreq   r3, r3, #F_Z
    tst     r0, #0x80
    it      ne
    orrne   r3, r3, #F_S
    cmp     r0, #0x7F
    it      eq
    orreq   r3, r3, #F_V
    ands    r1, r0, #0x0F
    cmp     r1, #0x0F
    it      eq
    orreq   r3, r3, #F_H
    and     r2, r2, #0xFF00
    orr     r2, r2, r3
    strh    r2, [r6, #CPU_AF]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x24: INC H (4 cycles) */
op_24_inc_h:
    ldrh    r5, [r6, #CPU_HL]
    lsrs    r0, r5, #8
    uxtb    r0, r0
    adds    r0, r0, #1
    uxtb    r0, r0
    bfi     r5, r0, #8, #8
    strh    r5, [r6, #CPU_HL]
    ldrh    r2, [r6, #CPU_AF]
    uxtb    r3, r2
    ands    r3, r3, #F_C
    cmp     r0, #0
    it      eq
    orreq   r3, r3, #F_Z
    tst     r0, #0x80
    it      ne
    orrne   r3, r3, #F_S
    cmp     r0, #0x80
    it      eq
    orreq   r3, r3, #F_V
    tst     r0, #0x0F
    it      eq
    orreq   r3, r3, #F_H
    and     r2, r2, #0xFF00
    orr     r2, r2, r3
    strh    r2, [r6, #CPU_AF]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x25: DEC H (4 cycles) */
op_25_dec_h:
    ldrh    r5, [r6, #CPU_HL]
    lsrs    r0, r5, #8
    uxtb    r0, r0
    subs    r0, r0, #1
    uxtb    r0, r0
    bfi     r5, r0, #8, #8
    strh    r5, [r6, #CPU_HL]
    ldrh    r2, [r6, #CPU_AF]
    uxtb    r3, r2
    ands    r3, r3, #F_C
    orr     r3, r3, #F_N
    cmp     r0, #0
    it      eq
    orreq   r3, r3, #F_Z
    tst     r0, #0x80
    it      ne
    orrne   r3, r3, #F_S
    cmp     r0, #0x7F
    it      eq
    orreq   r3, r3, #F_V
    ands    r1, r0, #0x0F
    cmp     r1, #0x0F
    it      eq
    orreq   r3, r3, #F_H
    and     r2, r2, #0xFF00
    orr     r2, r2, r3
    strh    r2, [r6, #CPU_AF]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x2C: INC L (4 cycles) */
op_2c_inc_l:
    ldrh    r5, [r6, #CPU_HL]
    uxtb    r0, r5
    adds    r0, r0, #1
    uxtb    r0, r0
    bfi     r5, r0, #0, #8
    strh    r5, [r6, #CPU_HL]
    ldrh    r2, [r6, #CPU_AF]
    uxtb    r3, r2
    ands    r3, r3, #F_C
    cmp     r0, #0
    it      eq
    orreq   r3, r3, #F_Z
    tst     r0, #0x80
    it      ne
    orrne   r3, r3, #F_S
    cmp     r0, #0x80
    it      eq
    orreq   r3, r3, #F_V
    tst     r0, #0x0F
    it      eq
    orreq   r3, r3, #F_H
    and     r2, r2, #0xFF00
    orr     r2, r2, r3
    strh    r2, [r6, #CPU_AF]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x2D: DEC L (4 cycles) */
op_2d_dec_l:
    ldrh    r5, [r6, #CPU_HL]
    uxtb    r0, r5
    subs    r0, r0, #1
    uxtb    r0, r0
    bfi     r5, r0, #0, #8
    strh    r5, [r6, #CPU_HL]
    ldrh    r2, [r6, #CPU_AF]
    uxtb    r3, r2
    ands    r3, r3, #F_C
    orr     r3, r3, #F_N
    cmp     r0, #0
    it      eq
    orreq   r3, r3, #F_Z
    tst     r0, #0x80
    it      ne
    orrne   r3, r3, #F_S
    cmp     r0, #0x7F
    it      eq
    orreq   r3, r3, #F_V
    ands    r1, r0, #0x0F
    cmp     r1, #0x0F
    it      eq
    orreq   r3, r3, #F_H
    and     r2, r2, #0xFF00
    orr     r2, r2, r3
    strh    r2, [r6, #CPU_AF]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x34: INC (HL) (11 cycles) */
op_34_inc_hl:
    ldrh    r5, [r6, #CPU_HL]
    uxth    r5, r5
    mov     r0, r5
    bl      RdZ80
    uxtb    r0, r0
    adds    r0, r0, #1
    uxtb    r0, r0
    ldrh    r2, [r6, #CPU_AF]
    uxtb    r3, r2
    ands    r3, r3, #F_C
    cmp     r0, #0
    it      eq
    orreq   r3, r3, #F_Z
    tst     r0, #0x80
    it      ne
    orrne   r3, r3, #F_S
    cmp     r0, #0x80
    it      eq
    orreq   r3, r3, #F_V
    tst     r0, #0x0F
    it      eq
    orreq   r3, r3, #F_H
    and     r2, r2, #0xFF00
    orr     r2, r2, r3
    strh    r2, [r6, #CPU_AF]
    mov     r1, r0
    mov     r0, r5
    bl      WrZ80
    add     r7, r7, #1
    subs    r4, r4, #11
    bgt     .Lloop
    b       .Lreturn

/* 0x35: DEC (HL) (11 cycles) */
op_35_dec_hl:
    ldrh    r5, [r6, #CPU_HL]
    uxth    r5, r5
    mov     r0, r5
    bl      RdZ80
    uxtb    r0, r0
    subs    r0, r0, #1
    uxtb    r0, r0
    ldrh    r2, [r6, #CPU_AF]
    uxtb    r3, r2
    ands    r3, r3, #F_C
    orr     r3, r3, #F_N
    cmp     r0, #0
    it      eq
    orreq   r3, r3, #F_Z
    tst     r0, #0x80
    it      ne
    orrne   r3, r3, #F_S
    cmp     r0, #0x7F
    it      eq
    orreq   r3, r3, #F_V
    ands    r1, r0, #0x0F
    cmp     r1, #0x0F
    it      eq
    orreq   r3, r3, #F_H
    and     r2, r2, #0xFF00
    orr     r2, r2, r3
    strh    r2, [r6, #CPU_AF]
    mov     r1, r0
    mov     r0, r5
    bl      WrZ80
    add     r7, r7, #1
    subs    r4, r4, #11
    bgt     .Lloop
    b       .Lreturn

/* 0x3C: INC A (4 cycles) */
op_3c_inc_a:
    ldrh    r5, [r6, #CPU_AF]
    lsrs    r0, r5, #8
    uxtb    r0, r0
    adds    r0, r0, #1
    uxtb    r0, r0
    ldrh    r2, [r6, #CPU_AF]
    uxtb    r3, r2
    ands    r3, r3, #F_C
    cmp     r0, #0
    it      eq
    orreq   r3, r3, #F_Z
    tst     r0, #0x80
    it      ne
    orrne   r3, r3, #F_S
    cmp     r0, #0x80
    it      eq
    orreq   r3, r3, #F_V
    tst     r0, #0x0F
    it      eq
    orreq   r3, r3, #F_H
    orr     r2, r3, r0, lsl #8
    strh    r2, [r6, #CPU_AF]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x3D: DEC A (4 cycles) */
op_3d_dec_a:
    ldrh    r5, [r6, #CPU_AF]
    lsrs    r0, r5, #8
    uxtb    r0, r0
    subs    r0, r0, #1
    uxtb    r0, r0
    ldrh    r2, [r6, #CPU_AF]
    uxtb    r3, r2
    ands    r3, r3, #F_C
    orr     r3, r3, #F_N
    cmp     r0, #0
    it      eq
    orreq   r3, r3, #F_Z
    tst     r0, #0x80
    it      ne
    orrne   r3, r3, #F_S
    cmp     r0, #0x7F
    it      eq
    orreq   r3, r3, #F_V
    ands    r1, r0, #0x0F
    cmp     r1, #0x0F
    it      eq
    orreq   r3, r3, #F_H
    orr     r2, r3, r0, lsl #8
    strh    r2, [r6, #CPU_AF]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x13: INC DE (6 cycles) */
op_13_inc_de:
    ldrh    r5, [r6, #CPU_DE]
    add     r5, r5, #1
    uxth    r5, r5
    strh    r5, [r6, #CPU_DE]
    add     r7, r7, #1
    subs    r4, r4, #6
    bgt     .Lloop
    b       .Lreturn

/* 0x1B: DEC DE (6 cycles) */
op_1b_dec_de:
    ldrh    r5, [r6, #CPU_DE]
    subs    r5, r5, #1
    uxth    r5, r5
    strh    r5, [r6, #CPU_DE]
    add     r7, r7, #1
    subs    r4, r4, #6
    bgt     .Lloop
    b       .Lreturn

/* 0x23: INC HL (6 cycles) */
op_23_inc_hl:
    ldrh    r5, [r6, #CPU_HL]
    add     r5, r5, #1
    uxth    r5, r5
    strh    r5, [r6, #CPU_HL]
    add     r7, r7, #1
    subs    r4, r4, #6
    bgt     .Lloop
    b       .Lreturn

/* 0x2B: DEC HL (6 cycles) */
op_2b_dec_hl:
    ldrh    r5, [r6, #CPU_HL]
    subs    r5, r5, #1
    uxth    r5, r5
    strh    r5, [r6, #CPU_HL]
    add     r7, r7, #1
    subs    r4, r4, #6
    bgt     .Lloop
    b       .Lreturn

/* 0x33: INC SP (6 cycles) */
op_33_inc_sp:
    ldrh    r5, [r6, #CPU_SP]
    add     r5, r5, #1
    uxth    r5, r5
    strh    r5, [r6, #CPU_SP]
    add     r7, r7, #1
    subs    r4, r4, #6
    bgt     .Lloop
    b       .Lreturn

/* 0x3B: DEC SP (6 cycles) */
op_3b_dec_sp:
    ldrh    r5, [r6, #CPU_SP]
    subs    r5, r5, #1
    uxth    r5, r5
    strh    r5, [r6, #CPU_SP]
    add     r7, r7, #1
    subs    r4, r4, #6
    bgt     .Lloop
    b       .Lreturn

/* 0x3E: LD A,n (7 cycles) */
op_3e_ld_a_n:
    add     r0, r7, #1         /* PC+1 */
    uxth    r0, r0
    bl      RdZ80               /* r0 = imm */
    ldrh    r5, [r6, #CPU_AF]
    bfi     r5, r0, #8, #8      /* set A */
    strh    r5, [r6, #CPU_AF]
    add     r7, r7, #2         /* opcode + imm */
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

/* 0x21: LD HL,nn (10 cycles) */
op_21_ld_hl_nn:
    add     r0, r7, #1         /* low */
    uxth    r0, r0
    bl      RdZ80
    mov     r5, r0
    add     r0, r7, #2         /* high */
    uxth    r0, r0
    bl      RdZ80
    orr     r5, r5, r0, lsl #8
    uxth    r5, r5
    strh    r5, [r6, #CPU_HL]
    add     r7, r7, #3         /* opcode + imm16 */
    subs    r4, r4, #10
    bgt     .Lloop
    b       .Lreturn

/* 0x06: LD B,n (7 cycles) */
op_06_ld_b_n:
    add     r0, r7, #1
    uxth    r0, r0
    bl      RdZ80
    uxtb    r0, r0
    ldrh    r5, [r6, #CPU_BC]
    bfi     r5, r0, #8, #8
    strh    r5, [r6, #CPU_BC]
    add     r7, r7, #2
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

/* 0x0E: LD C,n (7 cycles) */
op_0e_ld_c_n:
    add     r0, r7, #1
    uxth    r0, r0
    bl      RdZ80
    uxtb    r0, r0
    ldrh    r5, [r6, #CPU_BC]
    bfi     r5, r0, #0, #8
    strh    r5, [r6, #CPU_BC]
    add     r7, r7, #2
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

/* 0x16: LD D,n (7 cycles) */
op_16_ld_d_n:
    add     r0, r7, #1
    uxth    r0, r0
    bl      RdZ80
    uxtb    r0, r0
    ldrh    r5, [r6, #CPU_DE]
    bfi     r5, r0, #8, #8
    strh    r5, [r6, #CPU_DE]
    add     r7, r7, #2
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

/* 0x1E: LD E,n (7 cycles) */
op_1e_ld_e_n:
    add     r0, r7, #1
    uxth    r0, r0
    bl      RdZ80
    uxtb    r0, r0
    ldrh    r5, [r6, #CPU_DE]
    bfi     r5, r0, #0, #8
    strh    r5, [r6, #CPU_DE]
    add     r7, r7, #2
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

/* 0x26: LD H,n (7 cycles) */
op_26_ld_h_n:
    add     r0, r7, #1
    uxth    r0, r0
    bl      RdZ80
    uxtb    r0, r0
    ldrh    r5, [r6, #CPU_HL]
    bfi     r5, r0, #8, #8
    strh    r5, [r6, #CPU_HL]
    add     r7, r7, #2
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

/* 0x2E: LD L,n (7 cycles) */
op_2e_ld_l_n:
    add     r0, r7, #1
    uxth    r0, r0
    bl      RdZ80
    uxtb    r0, r0
    ldrh    r5, [r6, #CPU_HL]
    bfi     r5, r0, #0, #8
    strh    r5, [r6, #CPU_HL]
    add     r7, r7, #2
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

/* 0x36: LD (HL),n (10 cycles) */
op_36_ld_hl_n:
    ldrh    r5, [r6, #CPU_HL]
    uxth    r5, r5
    add     r0, r7, #1
    uxth    r0, r0
    bl      RdZ80
    uxtb    r1, r0
    mov     r0, r5
    bl      WrZ80
    add     r7, r7, #2
    subs    r4, r4, #10
    bgt     .Lloop
    b       .Lreturn

/* 0x32: LD (nn),A (13 cycles) */
op_32_ld_nn_a:
    add     r0, r7, #1         /* low */
    uxth    r0, r0
    bl      RdZ80
    mov     r5, r0
    add     r0, r7, #2         /* high */
    uxth    r0, r0
    bl      RdZ80
    orr     r5, r5, r0, lsl #8
    uxth    r5, r5
    ldrh    r2, [r6, #CPU_AF]
    lsrs    r1, r2, #8
    uxtb    r1, r1
    mov     r0, r5
    bl      WrZ80
    add     r7, r7, #3
    subs    r4, r4, #13
    bgt     .Lloop
    b       .Lreturn

/* 0x3A: LD A,(nn) (13 cycles) */
op_3a_ld_a_nn:
    add     r0, r7, #1         /* low */
    uxth    r0, r0
    bl      RdZ80
    mov     r5, r0
    add     r0, r7, #2         /* high */
    uxth    r0, r0
    bl      RdZ80
    orr     r5, r5, r0, lsl #8
    uxth    r5, r5
    mov     r0, r5
    bl      RdZ80
    uxtb    r0, r0
    ldrh    r5, [r6, #CPU_AF]
    bfi     r5, r0, #8, #8
    strh    r5, [r6, #CPU_AF]
    add     r7, r7, #3
    subs    r4, r4, #13
    bgt     .Lloop
    b       .Lreturn

/* 0x08: EX AF,AF' (4 cycles) */
op_08_ex_af_af1:
    ldrh    r5, [r6, #CPU_AF]
    ldrh    r2, [r6, #CPU_AF1]
    strh    r2, [r6, #CPU_AF]
    strh    r5, [r6, #CPU_AF1]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0xEB: EX DE,HL (4 cycles) */
op_eb_ex_de_hl:
    ldrh    r5, [r6, #CPU_DE]
    ldrh    r2, [r6, #CPU_HL]
    strh    r2, [r6, #CPU_DE]
    strh    r5, [r6, #CPU_HL]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0xD9: EXX (4 cycles) */
op_d9_exx:
    ldrh    r5, [r6, #CPU_BC]
    ldrh    r2, [r6, #CPU_BC1]
    strh    r2, [r6, #CPU_BC]
    strh    r5, [r6, #CPU_BC1]
    ldrh    r5, [r6, #CPU_DE]
    ldrh    r2, [r6, #CPU_DE1]
    strh    r2, [r6, #CPU_DE]
    strh    r5, [r6, #CPU_DE1]
    ldrh    r5, [r6, #CPU_HL]
    ldrh    r2, [r6, #CPU_HL1]
    strh    r2, [r6, #CPU_HL]
    strh    r5, [r6, #CPU_HL1]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0xF9: LD SP,HL (6 cycles) */
op_f9_ld_sp_hl:
    ldrh    r5, [r6, #CPU_HL]
    strh    r5, [r6, #CPU_SP]
    add     r7, r7, #1
    subs    r4, r4, #6
    bgt     .Lloop
    b       .Lreturn

/* 0xE3: EX (SP),HL (19 cycles) */
op_e3_ex_sp_hl:
    /* Read word at (SP) */
    ldrh    r2, [r6, #CPU_SP]
    uxth    r2, r2
    mov     r0, r2
    bl      RdZ80
    uxtb    r5, r0             /* low */
    ldrh    r2, [r6, #CPU_SP]
    add     r2, r2, #1
    uxth    r2, r2
    mov     r0, r2
    bl      RdZ80
    uxtb    r0, r0             /* high */
    orr     r5, r5, r0, lsl #8
    uxth    r5, r5

    /* Write HL back to (SP) */
    ldrh    r3, [r6, #CPU_HL]
    uxtb    r1, r3             /* low */
    ldrh    r2, [r6, #CPU_SP]
    uxth    r2, r2
    mov     r0, r2
    bl      WrZ80
    ldrh    r3, [r6, #CPU_HL]
    lsrs    r1, r3, #8         /* high */
    uxtb    r1, r1
    ldrh    r2, [r6, #CPU_SP]
    add     r0, r2, #1
    uxth    r0, r0
    bl      WrZ80

    /* HL = old (SP) */
    strh    r5, [r6, #CPU_HL]
    add     r7, r7, #1
    subs    r4, r4, #19
    bgt     .Lloop
    b       .Lreturn

/* 0xC1: POP BC (10 cycles) */
op_c1_pop_bc:
    /* NOTE: RdZ80 clobbers r0-r3, so never use r1/r2 after calls. */
    ldrh    r2, [r6, #CPU_SP]
    uxth    r2, r2
    mov     r0, r2
    bl      RdZ80
    uxtb    r5, r0             /* low */
    ldrh    r2, [r6, #CPU_SP]
    add     r2, r2, #1
    uxth    r2, r2
    mov     r0, r2
    bl      RdZ80
    uxtb    r0, r0             /* high */
    orr     r5, r5, r0, lsl #8
    strh    r5, [r6, #CPU_BC]
    ldrh    r2, [r6, #CPU_SP]
    add     r2, r2, #2
    uxth    r2, r2
    strh    r2, [r6, #CPU_SP]
    add     r7, r7, #1
    subs    r4, r4, #10
    bgt     .Lloop
    b       .Lreturn

/* 0xD1: POP DE (10 cycles) */
op_d1_pop_de:
    ldrh    r2, [r6, #CPU_SP]
    uxth    r2, r2
    mov     r0, r2
    bl      RdZ80
    uxtb    r5, r0
    ldrh    r2, [r6, #CPU_SP]
    add     r2, r2, #1
    uxth    r2, r2
    mov     r0, r2
    bl      RdZ80
    uxtb    r0, r0
    orr     r5, r5, r0, lsl #8
    strh    r5, [r6, #CPU_DE]
    ldrh    r2, [r6, #CPU_SP]
    add     r2, r2, #2
    uxth    r2, r2
    strh    r2, [r6, #CPU_SP]
    add     r7, r7, #1
    subs    r4, r4, #10
    bgt     .Lloop
    b       .Lreturn

/* 0xE1: POP HL (10 cycles) */
op_e1_pop_hl:
    ldrh    r2, [r6, #CPU_SP]
    uxth    r2, r2
    mov     r0, r2
    bl      RdZ80
    uxtb    r5, r0
    ldrh    r2, [r6, #CPU_SP]
    add     r2, r2, #1
    uxth    r2, r2
    mov     r0, r2
    bl      RdZ80
    uxtb    r0, r0
    orr     r5, r5, r0, lsl #8
    strh    r5, [r6, #CPU_HL]
    ldrh    r2, [r6, #CPU_SP]
    add     r2, r2, #2
    uxth    r2, r2
    strh    r2, [r6, #CPU_SP]
    add     r7, r7, #1
    subs    r4, r4, #10
    bgt     .Lloop
    b       .Lreturn

/* 0xF1: POP AF (10 cycles) */
op_f1_pop_af:
    ldrh    r2, [r6, #CPU_SP]
    uxth    r2, r2
    mov     r0, r2
    bl      RdZ80
    uxtb    r5, r0
    ldrh    r2, [r6, #CPU_SP]
    add     r2, r2, #1
    uxth    r2, r2
    mov     r0, r2
    bl      RdZ80
    uxtb    r0, r0
    orr     r5, r5, r0, lsl #8
    strh    r5, [r6, #CPU_AF]
    ldrh    r2, [r6, #CPU_SP]
    add     r2, r2, #2
    uxth    r2, r2
    strh    r2, [r6, #CPU_SP]
    add     r7, r7, #1
    subs    r4, r4, #10
    bgt     .Lloop
    b       .Lreturn

/* 0xC5: PUSH BC (11 cycles) */
op_c5_push_bc:
    /* NOTE: WrZ80 clobbers r0-r3, so recompute addresses from CPU_SP. */
    ldrh    r2, [r6, #CPU_SP]
    uxth    r2, r2
    ldrh    r5, [r6, #CPU_BC]
    lsr     r1, r5, #8         /* high */
    uxtb    r1, r1
    subs    r0, r2, #1         /* addr = SP-1 */
    uxth    r0, r0
    bl      WrZ80
    uxtb    r1, r5             /* low */
    ldrh    r2, [r6, #CPU_SP]
    subs    r0, r2, #2         /* addr = SP-2 */
    uxth    r0, r0
    bl      WrZ80
    ldrh    r2, [r6, #CPU_SP]
    subs    r2, r2, #2
    uxth    r2, r2
    strh    r2, [r6, #CPU_SP]
    add     r7, r7, #1
    subs    r4, r4, #11
    bgt     .Lloop
    b       .Lreturn

/* 0xD5: PUSH DE (11 cycles) */
op_d5_push_de:
    ldrh    r2, [r6, #CPU_SP]
    uxth    r2, r2
    ldrh    r5, [r6, #CPU_DE]
    lsr     r1, r5, #8         /* high */
    uxtb    r1, r1
    subs    r0, r2, #1         /* addr = SP-1 */
    uxth    r0, r0
    bl      WrZ80
    uxtb    r1, r5             /* low */
    ldrh    r2, [r6, #CPU_SP]
    subs    r0, r2, #2         /* addr = SP-2 */
    uxth    r0, r0
    bl      WrZ80
    ldrh    r2, [r6, #CPU_SP]
    subs    r2, r2, #2
    uxth    r2, r2
    strh    r2, [r6, #CPU_SP]
    add     r7, r7, #1
    subs    r4, r4, #11
    bgt     .Lloop
    b       .Lreturn

/* 0xE5: PUSH HL (11 cycles) */
op_e5_push_hl:
    ldrh    r2, [r6, #CPU_SP]
    uxth    r2, r2
    ldrh    r5, [r6, #CPU_HL]
    lsr     r1, r5, #8         /* high */
    uxtb    r1, r1
    subs    r0, r2, #1         /* addr = SP-1 */
    uxth    r0, r0
    bl      WrZ80
    uxtb    r1, r5             /* low */
    ldrh    r2, [r6, #CPU_SP]
    subs    r0, r2, #2         /* addr = SP-2 */
    uxth    r0, r0
    bl      WrZ80
    ldrh    r2, [r6, #CPU_SP]
    subs    r2, r2, #2
    uxth    r2, r2
    strh    r2, [r6, #CPU_SP]
    add     r7, r7, #1
    subs    r4, r4, #11
    bgt     .Lloop
    b       .Lreturn

/* 0xF5: PUSH AF (11 cycles) */
op_f5_push_af:
    ldrh    r2, [r6, #CPU_SP]
    uxth    r2, r2
    ldrh    r5, [r6, #CPU_AF]
    lsr     r1, r5, #8         /* high */
    uxtb    r1, r1
    subs    r0, r2, #1         /* addr = SP-1 */
    uxth    r0, r0
    bl      WrZ80
    uxtb    r1, r5             /* low */
    ldrh    r2, [r6, #CPU_SP]
    subs    r0, r2, #2         /* addr = SP-2 */
    uxth    r0, r0
    bl      WrZ80
    ldrh    r2, [r6, #CPU_SP]
    subs    r2, r2, #2
    uxth    r2, r2
    strh    r2, [r6, #CPU_SP]
    add     r7, r7, #1
    subs    r4, r4, #11
    bgt     .Lloop
    b       .Lreturn

/* 0x46: LD B,(HL) (7 cycles) */
op_46_ld_b_hl:
    ldrh    r0, [r6, #CPU_HL]
    uxth    r0, r0
    bl      RdZ80
    uxtb    r0, r0
    ldrh    r5, [r6, #CPU_BC]
    bfi     r5, r0, #8, #8
    strh    r5, [r6, #CPU_BC]
    add     r7, r7, #1
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

/* 0x4E: LD C,(HL) (7 cycles) */
op_4e_ld_c_hl:
    ldrh    r0, [r6, #CPU_HL]
    uxth    r0, r0
    bl      RdZ80
    uxtb    r0, r0
    ldrh    r5, [r6, #CPU_BC]
    bfi     r5, r0, #0, #8
    strh    r5, [r6, #CPU_BC]
    add     r7, r7, #1
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

/* 0x56: LD D,(HL) (7 cycles) */
op_56_ld_d_hl:
    ldrh    r0, [r6, #CPU_HL]
    uxth    r0, r0
    bl      RdZ80
    uxtb    r0, r0
    ldrh    r5, [r6, #CPU_DE]
    bfi     r5, r0, #8, #8
    strh    r5, [r6, #CPU_DE]
    add     r7, r7, #1
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

/* 0x5E: LD E,(HL) (7 cycles) */
op_5e_ld_e_hl:
    ldrh    r0, [r6, #CPU_HL]
    uxth    r0, r0
    bl      RdZ80
    uxtb    r0, r0
    ldrh    r5, [r6, #CPU_DE]
    bfi     r5, r0, #0, #8
    strh    r5, [r6, #CPU_DE]
    add     r7, r7, #1
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

/* 0x66: LD H,(HL) (7 cycles) */
op_66_ld_h_hl:
    ldrh    r0, [r6, #CPU_HL]
    uxth    r0, r0
    bl      RdZ80
    uxtb    r0, r0
    ldrh    r5, [r6, #CPU_HL]
    bfi     r5, r0, #8, #8
    strh    r5, [r6, #CPU_HL]
    add     r7, r7, #1
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

/* 0x6E: LD L,(HL) (7 cycles) */
op_6e_ld_l_hl:
    ldrh    r0, [r6, #CPU_HL]
    uxth    r0, r0
    bl      RdZ80
    uxtb    r0, r0
    ldrh    r5, [r6, #CPU_HL]
    bfi     r5, r0, #0, #8
    strh    r5, [r6, #CPU_HL]
    add     r7, r7, #1
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

/* 0x40: LD B,B (4 cycles) */
op_40_ld_b_b:
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x41: LD B,C (4 cycles) */
op_41_ld_b_c:
    ldrh    r5, [r6, #CPU_BC]
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_BC]
    bfi     r1, r5, #8, #8
    strh    r1, [r6, #CPU_BC]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x42: LD B,D (4 cycles) */
op_42_ld_b_d:
    ldrh    r5, [r6, #CPU_DE]
    lsr     r5, r5, #8
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_BC]
    bfi     r1, r5, #8, #8
    strh    r1, [r6, #CPU_BC]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x43: LD B,E (4 cycles) */
op_43_ld_b_e:
    ldrh    r5, [r6, #CPU_DE]
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_BC]
    bfi     r1, r5, #8, #8
    strh    r1, [r6, #CPU_BC]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x44: LD B,H (4 cycles) */
op_44_ld_b_h:
    ldrh    r5, [r6, #CPU_HL]
    lsr     r5, r5, #8
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_BC]
    bfi     r1, r5, #8, #8
    strh    r1, [r6, #CPU_BC]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x45: LD B,L (4 cycles) */
op_45_ld_b_l:
    ldrh    r5, [r6, #CPU_HL]
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_BC]
    bfi     r1, r5, #8, #8
    strh    r1, [r6, #CPU_BC]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x47: LD B,A (4 cycles) */
op_47_ld_b_a:
    ldrh    r5, [r6, #CPU_AF]
    lsr     r5, r5, #8
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_BC]
    bfi     r1, r5, #8, #8
    strh    r1, [r6, #CPU_BC]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x48: LD C,B (4 cycles) */
op_48_ld_c_b:
    ldrh    r5, [r6, #CPU_BC]
    lsr     r5, r5, #8
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_BC]
    bfi     r1, r5, #0, #8
    strh    r1, [r6, #CPU_BC]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x49: LD C,C (4 cycles) */
op_49_ld_c_c:
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x4A: LD C,D (4 cycles) */
op_4a_ld_c_d:
    ldrh    r5, [r6, #CPU_DE]
    lsr     r5, r5, #8
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_BC]
    bfi     r1, r5, #0, #8
    strh    r1, [r6, #CPU_BC]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x4B: LD C,E (4 cycles) */
op_4b_ld_c_e:
    ldrh    r5, [r6, #CPU_DE]
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_BC]
    bfi     r1, r5, #0, #8
    strh    r1, [r6, #CPU_BC]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x4C: LD C,H (4 cycles) */
op_4c_ld_c_h:
    ldrh    r5, [r6, #CPU_HL]
    lsr     r5, r5, #8
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_BC]
    bfi     r1, r5, #0, #8
    strh    r1, [r6, #CPU_BC]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x4D: LD C,L (4 cycles) */
op_4d_ld_c_l:
    ldrh    r5, [r6, #CPU_HL]
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_BC]
    bfi     r1, r5, #0, #8
    strh    r1, [r6, #CPU_BC]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x4F: LD C,A (4 cycles) */
op_4f_ld_c_a:
    ldrh    r5, [r6, #CPU_AF]
    lsr     r5, r5, #8
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_BC]
    bfi     r1, r5, #0, #8
    strh    r1, [r6, #CPU_BC]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x50: LD D,B (4 cycles) */
op_50_ld_d_b:
    ldrh    r5, [r6, #CPU_BC]
    lsr     r5, r5, #8
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_DE]
    bfi     r1, r5, #8, #8
    strh    r1, [r6, #CPU_DE]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x51: LD D,C (4 cycles) */
op_51_ld_d_c:
    ldrh    r5, [r6, #CPU_BC]
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_DE]
    bfi     r1, r5, #8, #8
    strh    r1, [r6, #CPU_DE]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x52: LD D,D (4 cycles) */
op_52_ld_d_d:
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x53: LD D,E (4 cycles) */
op_53_ld_d_e:
    ldrh    r5, [r6, #CPU_DE]
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_DE]
    bfi     r1, r5, #8, #8
    strh    r1, [r6, #CPU_DE]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x54: LD D,H (4 cycles) */
op_54_ld_d_h:
    ldrh    r5, [r6, #CPU_HL]
    lsr     r5, r5, #8
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_DE]
    bfi     r1, r5, #8, #8
    strh    r1, [r6, #CPU_DE]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x55: LD D,L (4 cycles) */
op_55_ld_d_l:
    ldrh    r5, [r6, #CPU_HL]
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_DE]
    bfi     r1, r5, #8, #8
    strh    r1, [r6, #CPU_DE]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x57: LD D,A (4 cycles) */
op_57_ld_d_a:
    ldrh    r5, [r6, #CPU_AF]
    lsr     r5, r5, #8
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_DE]
    bfi     r1, r5, #8, #8
    strh    r1, [r6, #CPU_DE]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x58: LD E,B (4 cycles) */
op_58_ld_e_b:
    ldrh    r5, [r6, #CPU_BC]
    lsr     r5, r5, #8
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_DE]
    bfi     r1, r5, #0, #8
    strh    r1, [r6, #CPU_DE]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x59: LD E,C (4 cycles) */
op_59_ld_e_c:
    ldrh    r5, [r6, #CPU_BC]
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_DE]
    bfi     r1, r5, #0, #8
    strh    r1, [r6, #CPU_DE]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x5A: LD E,D (4 cycles) */
op_5a_ld_e_d:
    ldrh    r5, [r6, #CPU_DE]
    lsr     r5, r5, #8
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_DE]
    bfi     r1, r5, #0, #8
    strh    r1, [r6, #CPU_DE]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x5B: LD E,E (4 cycles) */
op_5b_ld_e_e:
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x5C: LD E,H (4 cycles) */
op_5c_ld_e_h:
    ldrh    r5, [r6, #CPU_HL]
    lsr     r5, r5, #8
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_DE]
    bfi     r1, r5, #0, #8
    strh    r1, [r6, #CPU_DE]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x5D: LD E,L (4 cycles) */
op_5d_ld_e_l:
    ldrh    r5, [r6, #CPU_HL]
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_DE]
    bfi     r1, r5, #0, #8
    strh    r1, [r6, #CPU_DE]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x5F: LD E,A (4 cycles) */
op_5f_ld_e_a:
    ldrh    r5, [r6, #CPU_AF]
    lsr     r5, r5, #8
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_DE]
    bfi     r1, r5, #0, #8
    strh    r1, [r6, #CPU_DE]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x60: LD H,B (4 cycles) */
op_60_ld_h_b:
    ldrh    r5, [r6, #CPU_BC]
    lsr     r5, r5, #8
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_HL]
    bfi     r1, r5, #8, #8
    strh    r1, [r6, #CPU_HL]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x61: LD H,C (4 cycles) */
op_61_ld_h_c:
    ldrh    r5, [r6, #CPU_BC]
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_HL]
    bfi     r1, r5, #8, #8
    strh    r1, [r6, #CPU_HL]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x62: LD H,D (4 cycles) */
op_62_ld_h_d:
    ldrh    r5, [r6, #CPU_DE]
    lsr     r5, r5, #8
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_HL]
    bfi     r1, r5, #8, #8
    strh    r1, [r6, #CPU_HL]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x63: LD H,E (4 cycles) */
op_63_ld_h_e:
    ldrh    r5, [r6, #CPU_DE]
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_HL]
    bfi     r1, r5, #8, #8
    strh    r1, [r6, #CPU_HL]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x64: LD H,H (4 cycles) */
op_64_ld_h_h:
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x65: LD H,L (4 cycles) */
op_65_ld_h_l:
    ldrh    r5, [r6, #CPU_HL]
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_HL]
    bfi     r1, r5, #8, #8
    strh    r1, [r6, #CPU_HL]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x67: LD H,A (4 cycles) */
op_67_ld_h_a:
    ldrh    r5, [r6, #CPU_AF]
    lsr     r5, r5, #8
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_HL]
    bfi     r1, r5, #8, #8
    strh    r1, [r6, #CPU_HL]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x68: LD L,B (4 cycles) */
op_68_ld_l_b:
    ldrh    r5, [r6, #CPU_BC]
    lsr     r5, r5, #8
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_HL]
    bfi     r1, r5, #0, #8
    strh    r1, [r6, #CPU_HL]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x69: LD L,C (4 cycles) */
op_69_ld_l_c:
    ldrh    r5, [r6, #CPU_BC]
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_HL]
    bfi     r1, r5, #0, #8
    strh    r1, [r6, #CPU_HL]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x6A: LD L,D (4 cycles) */
op_6a_ld_l_d:
    ldrh    r5, [r6, #CPU_DE]
    lsr     r5, r5, #8
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_HL]
    bfi     r1, r5, #0, #8
    strh    r1, [r6, #CPU_HL]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x6B: LD L,E (4 cycles) */
op_6b_ld_l_e:
    ldrh    r5, [r6, #CPU_DE]
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_HL]
    bfi     r1, r5, #0, #8
    strh    r1, [r6, #CPU_HL]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x6C: LD L,H (4 cycles) */
op_6c_ld_l_h:
    ldrh    r5, [r6, #CPU_HL]
    lsr     r5, r5, #8
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_HL]
    bfi     r1, r5, #0, #8
    strh    r1, [r6, #CPU_HL]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x6D: LD L,L (4 cycles) */
op_6d_ld_l_l:
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x6F: LD L,A (4 cycles) */
op_6f_ld_l_a:
    ldrh    r5, [r6, #CPU_AF]
    lsr     r5, r5, #8
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_HL]
    bfi     r1, r5, #0, #8
    strh    r1, [r6, #CPU_HL]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x78: LD A,B (4 cycles) */
op_78_ld_a_b:
    ldrh    r5, [r6, #CPU_BC]
    lsr     r5, r5, #8
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_AF]
    bfi     r1, r5, #8, #8
    strh    r1, [r6, #CPU_AF]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x79: LD A,C (4 cycles) */
op_79_ld_a_c:
    ldrh    r5, [r6, #CPU_BC]
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_AF]
    bfi     r1, r5, #8, #8
    strh    r1, [r6, #CPU_AF]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x7A: LD A,D (4 cycles) */
op_7a_ld_a_d:
    ldrh    r5, [r6, #CPU_DE]
    lsr     r5, r5, #8
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_AF]
    bfi     r1, r5, #8, #8
    strh    r1, [r6, #CPU_AF]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x7B: LD A,E (4 cycles) */
op_7b_ld_a_e:
    ldrh    r5, [r6, #CPU_DE]
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_AF]
    bfi     r1, r5, #8, #8
    strh    r1, [r6, #CPU_AF]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x7C: LD A,H (4 cycles) */
op_7c_ld_a_h:
    ldrh    r5, [r6, #CPU_HL]
    lsr     r5, r5, #8
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_AF]
    bfi     r1, r5, #8, #8
    strh    r1, [r6, #CPU_AF]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x7D: LD A,L (4 cycles) */
op_7d_ld_a_l:
    ldrh    r5, [r6, #CPU_HL]
    uxtb    r5, r5
    ldrh    r1, [r6, #CPU_AF]
    bfi     r1, r5, #8, #8
    strh    r1, [r6, #CPU_AF]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x7F: LD A,A (4 cycles) */
op_7f_ld_a_a:
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x80-0x87: ADD A,r/(HL) */
op_80_add_a_b:
    ldrh    r1, [r6, #CPU_BC]
    lsrs    r1, r1, #8
    uxtb    r1, r1
    b       .Ladd_a_r1_4
op_81_add_a_c:
    ldrh    r1, [r6, #CPU_BC]
    uxtb    r1, r1
    b       .Ladd_a_r1_4
op_82_add_a_d:
    ldrh    r1, [r6, #CPU_DE]
    lsrs    r1, r1, #8
    uxtb    r1, r1
    b       .Ladd_a_r1_4
op_83_add_a_e:
    ldrh    r1, [r6, #CPU_DE]
    uxtb    r1, r1
    b       .Ladd_a_r1_4
op_84_add_a_h:
    ldrh    r1, [r6, #CPU_HL]
    lsrs    r1, r1, #8
    uxtb    r1, r1
    b       .Ladd_a_r1_4
op_85_add_a_l:
    ldrh    r1, [r6, #CPU_HL]
    uxtb    r1, r1
    b       .Ladd_a_r1_4
op_86_add_a_hl:
    ldrh    r5, [r6, #CPU_HL]
    uxth    r0, r5
    bl      RdZ80
    uxtb    r1, r0
    b       .Ladd_a_r1_7
op_87_add_a_a:
    ldrh    r5, [r6, #CPU_AF]
    lsrs    r1, r5, #8
    uxtb    r1, r1
    b       .Ladd_a_r1_4

.Ladd_a_r1_common:
    /* r1 = operand */
    ldrh    r5, [r6, #CPU_AF]
    lsrs    r0, r5, #8         /* r0 = A */
    uxtb    r0, r0
    uxtb    r1, r1
    adds    r2, r0, r1         /* r2 = A+op, carry in CPSR */
    uxtb    r2, r2
    movs    r3, #0
    adc     r3, r3, #0         /* r3 = carry (0/1) */
    mov     r12, r3            /* flags start with C */

    /* Z,S from result */
    cmp     r2, #0
    it      eq
    orreq   r12, r12, #F_Z
    tst     r2, #0x80
    it      ne
    orrne   r12, r12, #F_S

    /* H: (A ^ op ^ res) & 0x10 */
    eor     r3, r0, r1
    eor     r3, r3, r2
    tst     r3, #0x10
    it      ne
    orrne   r12, r12, #F_H

    /* V: ~(A^op) & (op^res) & 0x80 */
    eor     r3, r0, r1
    mvn     r3, r3
    eor     r1, r1, r2         /* r1 = op^res */
    and     r3, r3, r1
    tst     r3, #0x80
    it      ne
    orrne   r12, r12, #F_V

    /* store AF */
    lsls    r5, r2, #8         /* A = result */
    orr     r5, r5, r12        /* F */
    strh    r5, [r6, #CPU_AF]
    bx      lr

.Ladd_a_r1_4:
    bl      .Ladd_a_r1_common
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

.Ladd_a_r1_7:
    bl      .Ladd_a_r1_common
    add     r7, r7, #1
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

/* 0x90-0x97: SUB r/(HL) */
op_90_sub_b:
    ldrh    r1, [r6, #CPU_BC]
    lsrs    r1, r1, #8
    uxtb    r1, r1
    b       .Lsub_r1_4
op_91_sub_c:
    ldrh    r1, [r6, #CPU_BC]
    uxtb    r1, r1
    b       .Lsub_r1_4
op_92_sub_d:
    ldrh    r1, [r6, #CPU_DE]
    lsrs    r1, r1, #8
    uxtb    r1, r1
    b       .Lsub_r1_4
op_93_sub_e:
    ldrh    r1, [r6, #CPU_DE]
    uxtb    r1, r1
    b       .Lsub_r1_4
op_94_sub_h:
    ldrh    r1, [r6, #CPU_HL]
    lsrs    r1, r1, #8
    uxtb    r1, r1
    b       .Lsub_r1_4
op_95_sub_l:
    ldrh    r1, [r6, #CPU_HL]
    uxtb    r1, r1
    b       .Lsub_r1_4
op_96_sub_hl:
    ldrh    r5, [r6, #CPU_HL]
    uxth    r0, r5
    bl      RdZ80
    uxtb    r1, r0
    b       .Lsub_r1_7
op_97_sub_a:
    ldrh    r5, [r6, #CPU_AF]
    lsrs    r1, r5, #8
    uxtb    r1, r1
    b       .Lsub_r1_4

.Lsub_r1_common:
    /* r1 = operand */
    ldrh    r5, [r6, #CPU_AF]
    lsrs    r0, r5, #8         /* r0 = A */
    uxtb    r0, r0
    uxtb    r1, r1
    subs    r2, r0, r1         /* r2 = A-op */
    uxtb    r2, r2
    movs    r3, #0
    adc     r3, r3, #0         /* r3 = noBorrow (0/1) */
    eor     r3, r3, #1         /* r3 = borrow (0/1), becomes C */
    mov     r12, r3
    orr     r12, r12, #F_N

    /* Z,S from result */
    cmp     r2, #0
    it      eq
    orreq   r12, r12, #F_Z
    tst     r2, #0x80
    it      ne
    orrne   r12, r12, #F_S

    /* H: (A ^ op ^ res) & 0x10 */
    eor     r3, r0, r1
    eor     r3, r3, r2
    tst     r3, #0x10
    it      ne
    orrne   r12, r12, #F_H

    /* V: (A^op) & (A^res) & 0x80 */
    eor     r3, r0, r1
    eor     r1, r0, r2         /* r1 = A^res */
    and     r3, r3, r1
    tst     r3, #0x80
    it      ne
    orrne   r12, r12, #F_V

    /* store AF */
    lsls    r5, r2, #8         /* A = result */
    orr     r5, r5, r12        /* F */
    strh    r5, [r6, #CPU_AF]
    bx      lr

.Lsub_r1_4:
    bl      .Lsub_r1_common
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

.Lsub_r1_7:
    bl      .Lsub_r1_common
    add     r7, r7, #1
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

/* 0x88-0x8F: ADC A,r/(HL) */
op_88_adc_a_b:
    ldrh    r1, [r6, #CPU_BC]
    lsrs    r1, r1, #8
    uxtb    r1, r1
    b       .Ladc_r1_4
op_89_adc_a_c:
    ldrh    r1, [r6, #CPU_BC]
    uxtb    r1, r1
    b       .Ladc_r1_4
op_8a_adc_a_d:
    ldrh    r1, [r6, #CPU_DE]
    lsrs    r1, r1, #8
    uxtb    r1, r1
    b       .Ladc_r1_4
op_8b_adc_a_e:
    ldrh    r1, [r6, #CPU_DE]
    uxtb    r1, r1
    b       .Ladc_r1_4
op_8c_adc_a_h:
    ldrh    r1, [r6, #CPU_HL]
    lsrs    r1, r1, #8
    uxtb    r1, r1
    b       .Ladc_r1_4
op_8d_adc_a_l:
    ldrh    r1, [r6, #CPU_HL]
    uxtb    r1, r1
    b       .Ladc_r1_4
op_8e_adc_a_hl:
    ldrh    r5, [r6, #CPU_HL]
    uxth    r0, r5
    bl      RdZ80
    uxtb    r1, r0
    b       .Ladc_r1_7
op_8f_adc_a_a:
    ldrh    r5, [r6, #CPU_AF]
    lsrs    r1, r5, #8
    uxtb    r1, r1
    b       .Ladc_r1_4

.Ladc_r1_common:
    /* r1 = operand */
    ldrh    r5, [r6, #CPU_AF]
    lsrs    r0, r5, #8         /* r0 = A */
    uxtb    r0, r0
    uxtb    r1, r1
    uxtb    r2, r5             /* r2 = F */
    and     r2, r2, #F_C       /* r2 = carry-in (0/1) */

    adds    r3, r0, r1         /* r3 = A + op */
    adds    r3, r3, r2         /* r3 = A + op + c */
    lsrs    r12, r3, #8        /* C out */
    uxtb    r3, r3             /* r3 = result */

    /* Z,S */
    cmp     r3, #0
    it      eq
    orreq   r12, r12, #F_Z
    tst     r3, #0x80
    it      ne
    orrne   r12, r12, #F_S

    /* H: (A ^ op ^ res) & 0x10 */
    eor     r2, r0, r1
    eor     r2, r2, r3
    tst     r2, #0x10
    it      ne
    orrne   r12, r12, #F_H

    /* V: ~(A^op) & (op^res) & 0x80 */
    eor     r2, r0, r1
    mvn     r2, r2
    eor     r1, r1, r3         /* r1 = op^res */
    and     r2, r2, r1
    tst     r2, #0x80
    it      ne
    orrne   r12, r12, #F_V

    /* store AF */
    lsls    r5, r3, #8
    orr     r5, r5, r12
    strh    r5, [r6, #CPU_AF]
    bx      lr

.Ladc_r1_4:
    bl      .Ladc_r1_common
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

.Ladc_r1_7:
    bl      .Ladc_r1_common
    add     r7, r7, #1
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

/* 0x98-0x9F: SBC A,r/(HL) */
op_98_sbc_a_b:
    ldrh    r1, [r6, #CPU_BC]
    lsrs    r1, r1, #8
    uxtb    r1, r1
    b       .Lsbc_r1_4
op_99_sbc_a_c:
    ldrh    r1, [r6, #CPU_BC]
    uxtb    r1, r1
    b       .Lsbc_r1_4
op_9a_sbc_a_d:
    ldrh    r1, [r6, #CPU_DE]
    lsrs    r1, r1, #8
    uxtb    r1, r1
    b       .Lsbc_r1_4
op_9b_sbc_a_e:
    ldrh    r1, [r6, #CPU_DE]
    uxtb    r1, r1
    b       .Lsbc_r1_4
op_9c_sbc_a_h:
    ldrh    r1, [r6, #CPU_HL]
    lsrs    r1, r1, #8
    uxtb    r1, r1
    b       .Lsbc_r1_4
op_9d_sbc_a_l:
    ldrh    r1, [r6, #CPU_HL]
    uxtb    r1, r1
    b       .Lsbc_r1_4
op_9e_sbc_a_hl:
    ldrh    r5, [r6, #CPU_HL]
    uxth    r0, r5
    bl      RdZ80
    uxtb    r1, r0
    b       .Lsbc_r1_7
op_9f_sbc_a_a:
    ldrh    r5, [r6, #CPU_AF]
    lsrs    r1, r5, #8
    uxtb    r1, r1
    b       .Lsbc_r1_4

.Lsbc_r1_common:
    /* r1 = operand */
    ldrh    r5, [r6, #CPU_AF]
    lsrs    r0, r5, #8         /* r0 = A */
    uxtb    r0, r0
    uxtb    r1, r1
    uxtb    r2, r5             /* r2 = F */
    and     r2, r2, #F_C       /* r2 = carry-in / borrow-in (0/1) */

    adds    r3, r1, r2         /* r3 = op + c */
    uxtb    r3, r3
    cmp     r0, r3
    ite     lo
    movlo   r12, #1            /* borrow */
    movhs   r12, #0
    subs    r3, r0, r3         /* r3 = A - (op+c) */
    uxtb    r3, r3

    orr     r12, r12, #F_N

    /* Z,S */
    cmp     r3, #0
    it      eq
    orreq   r12, r12, #F_Z
    tst     r3, #0x80
    it      ne
    orrne   r12, r12, #F_S

    /* H: (A ^ op ^ res) & 0x10 */
    eor     r2, r0, r1
    eor     r2, r2, r3
    tst     r2, #0x10
    it      ne
    orrne   r12, r12, #F_H

    /* V: (A^op) & (A^res) & 0x80 */
    eor     r2, r0, r1
    eor     r1, r0, r3         /* r1 = A^res */
    and     r2, r2, r1
    tst     r2, #0x80
    it      ne
    orrne   r12, r12, #F_V

    /* store AF */
    lsls    r5, r3, #8
    orr     r5, r5, r12
    strh    r5, [r6, #CPU_AF]
    bx      lr

.Lsbc_r1_4:
    bl      .Lsbc_r1_common
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

.Lsbc_r1_7:
    bl      .Lsbc_r1_common
    add     r7, r7, #1
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

/* Helpers: parity flag (even parity) */
.Lparity_flag:
    /* r0 = value (low 8 bits). returns r0 = 0 or F_V */
    uxtb    r0, r0
    eor     r1, r0, r0, lsr #4
    eor     r1, r1, r1, lsr #2
    eor     r1, r1, r1, lsr #1
    and     r1, r1, #1
    eor     r1, r1, #1         /* 1 if even parity */
    and     r0, r1, #1
    lsls    r0, r0, #2         /* -> 0x04 */
    bx      lr

/* Generic logic op: compute SZP into r12 (and optionally H) */
.Llogic_szp:
    /* r0 = result byte, returns r12 = flags SZP (no H/N/C)
       NOTE: must preserve r0 (result) for caller. */
    uxtb    r2, r0
    movs    r12, #0
    /* undocumented flags X/Y from result */
    and     r1, r2, #0x28
    orr     r12, r12, r1
    cmp     r2, #0
    it      eq
    orreq   r12, r12, #F_Z
    tst     r2, #0x80
    it      ne
    orrne   r12, r12, #F_S
    push    {lr}
    mov     r0, r2
    bl      .Lparity_flag
    mov     r3, r0
    pop     {lr}
    orr     r12, r12, r3
    mov     r0, r2
    bx      lr

/* 0x07: RLCA (4 cycles) */
op_07_rlca:
    ldrh    r5, [r6, #CPU_AF]
    lsrs    r0, r5, #8         /* A */
    uxtb    r0, r0
    and     r1, r5, #0xFF      /* F */
    lsrs    r2, r0, #7         /* new C (0/1) */
    lsls    r0, r0, #1
    uxtb    r0, r0
    orr     r0, r0, r2
    bic     r1, r1, #(F_C|F_N|F_H)
    orr     r1, r1, r2
    lsls    r5, r0, #8
    orr     r5, r5, r1
    strh    r5, [r6, #CPU_AF]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x0F: RRCA (4 cycles) */
op_0f_rrca:
    ldrh    r5, [r6, #CPU_AF]
    lsrs    r0, r5, #8         /* A */
    uxtb    r0, r0
    and     r1, r5, #0xFF      /* F */
    and     r2, r0, #1         /* new C */
    lsrs    r0, r0, #1
    lsls    r3, r2, #7
    orr     r0, r0, r3
    uxtb    r0, r0
    bic     r1, r1, #(F_C|F_N|F_H)
    orr     r1, r1, r2
    lsls    r5, r0, #8
    orr     r5, r5, r1
    strh    r5, [r6, #CPU_AF]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x17: RLA (4 cycles) */
op_17_rla:
    ldrh    r5, [r6, #CPU_AF]
    lsrs    r0, r5, #8         /* A */
    uxtb    r0, r0
    and     r1, r5, #0xFF      /* F */
    lsrs    r2, r0, #7         /* new C */
    and     r3, r1, #F_C       /* old C */
    lsls    r0, r0, #1
    uxtb    r0, r0
    orr     r0, r0, r3
    bic     r1, r1, #(F_C|F_N|F_H)
    orr     r1, r1, r2
    lsls    r5, r0, #8
    orr     r5, r5, r1
    strh    r5, [r6, #CPU_AF]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x1F: RRA (4 cycles) */
op_1f_rra:
    ldrh    r5, [r6, #CPU_AF]
    lsrs    r0, r5, #8         /* A */
    uxtb    r0, r0
    and     r1, r5, #0xFF      /* F */
    and     r2, r0, #1         /* new C */
    and     r3, r1, #F_C       /* old C */
    lsrs    r0, r0, #1
    lsls    r3, r3, #7
    orr     r0, r0, r3
    uxtb    r0, r0
    bic     r1, r1, #(F_C|F_N|F_H)
    orr     r1, r1, r2
    lsls    r5, r0, #8
    orr     r5, r5, r1
    strh    r5, [r6, #CPU_AF]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x2F: CPL (4 cycles) */
op_2f_cpl:
    ldrh    r5, [r6, #CPU_AF]
    lsrs    r0, r5, #8         /* A */
    mvn     r0, r0
    uxtb    r0, r0
    and     r1, r5, #0xFF      /* F */
    orr     r1, r1, #(F_N|F_H)
    lsls    r5, r0, #8
    orr     r5, r5, r1
    strh    r5, [r6, #CPU_AF]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x37: SCF (4 cycles) */
op_37_scf:
    ldrh    r5, [r6, #CPU_AF]
    and     r1, r5, #0xFF      /* F */
    orr     r1, r1, #F_C
    bic     r1, r1, #(F_N|F_H)
    bic     r5, r5, #0xFF
    orr     r5, r5, r1
    strh    r5, [r6, #CPU_AF]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x3F: CCF (4 cycles) */
op_3f_ccf:
    ldrh    r5, [r6, #CPU_AF]
    and     r1, r5, #0xFF      /* F */
    eor     r1, r1, #F_C
    bic     r1, r1, #(F_N|F_H)
    tst     r1, #F_C
    it      eq
    orreq   r1, r1, #F_H
    bic     r5, r5, #0xFF
    orr     r5, r5, r1
    strh    r5, [r6, #CPU_AF]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x27: DAA (4 cycles)
   Matches the existing DAATable semantics, including undocumented X/Y flags. */
op_27_daa:
    ldrh    r5, [r6, #CPU_AF]
    lsrs    r0, r5, #8         /* A (original) */
    uxtb    r0, r0
    and     r1, r5, #0xFF      /* F (original) */

    /* c_out = (F_C) || (A > 0x99) */
    and     r3, r1, #F_C       /* carry_in (0/1) */
    movs    r2, #0
    cmp     r0, #0x99
    it      hi
    movhi   r2, #1
    orr     r2, r2, r3         /* r2 = c_out (0/1) */

    /* adj = 0; if (H || (A&0x0F) > 9) adj |= 6; if (c_out) adj |= 0x60 */
    movs    r3, #0
    tst     r1, #F_H
    bne     1f
    and     r12, r0, #0x0F
    cmp     r12, #9
    ble     2f
1:
    orr     r3, r3, #0x06
2:
    cmp     r2, #0
    it      ne
    orrne   r3, r3, #0x60

    /* res = (N ? A-adj : A+adj) */
    mov     r12, r0            /* keep original A in r0 */
    tst     r1, #F_N
    ite     eq
    addeq   r12, r12, r3
    subne   r12, r12, r3
    uxtb    r12, r12

    /* h_out = ((A ^ res) & 0x10) != 0 */
    eor     r5, r0, r12
    and     r5, r5, #0x10

    /* flags_out = (res & 0x28) | SZPV | N | H | C */
    and     r3, r12, #0x28     /* X/Y */
    cmp     r12, #0
    it      eq
    orreq   r3, r3, #F_Z
    tst     r12, #0x80
    it      ne
    orrne   r3, r3, #F_S
    push    {lr}
    mov     r0, r12
    bl      .Lparity_flag
    pop     {lr}
    orr     r3, r3, r0         /* PV */
    tst     r1, #F_N
    it      ne
    orrne   r3, r3, #F_N
    cmp     r5, #0
    it      ne
    orrne   r3, r3, #F_H
    cmp     r2, #0
    it      ne
    orrne   r3, r3, #F_C

    /* store AF */
    lsls    r5, r12, #8
    orr     r5, r5, r3
    strh    r5, [r6, #CPU_AF]
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0xA0-0xA7: AND r/(HL) */
op_a0_and_b:
    ldrh    r1, [r6, #CPU_BC]
    lsrs    r1, r1, #8
    uxtb    r1, r1
    b       .Land_r1_4
op_a1_and_c:
    ldrh    r1, [r6, #CPU_BC]
    uxtb    r1, r1
    b       .Land_r1_4
op_a2_and_d:
    ldrh    r1, [r6, #CPU_DE]
    lsrs    r1, r1, #8
    uxtb    r1, r1
    b       .Land_r1_4
op_a3_and_e:
    ldrh    r1, [r6, #CPU_DE]
    uxtb    r1, r1
    b       .Land_r1_4
op_a4_and_h:
    ldrh    r1, [r6, #CPU_HL]
    lsrs    r1, r1, #8
    uxtb    r1, r1
    b       .Land_r1_4
op_a5_and_l:
    ldrh    r1, [r6, #CPU_HL]
    uxtb    r1, r1
    b       .Land_r1_4
op_a6_and_hl:
    ldrh    r5, [r6, #CPU_HL]
    uxth    r0, r5
    bl      RdZ80
    uxtb    r1, r0
    b       .Land_r1_7
op_a7_and_a:
    ldrh    r5, [r6, #CPU_AF]
    lsrs    r1, r5, #8
    uxtb    r1, r1
    b       .Land_r1_4

.Land_common:
    /* r1 = operand */
    ldrh    r5, [r6, #CPU_AF]
    lsrs    r0, r5, #8
    uxtb    r0, r0
    uxtb    r1, r1
    and     r0, r0, r1         /* r0 = A &= op */
    push    {lr}
    bl      .Llogic_szp
    pop     {lr}
    orr     r12, r12, #F_H     /* AND sets H */
    lsls    r5, r0, #8
    orr     r5, r5, r12
    strh    r5, [r6, #CPU_AF]
    bx      lr

.Land_r1_4:
    bl      .Land_common
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

.Land_r1_7:
    bl      .Land_common
    add     r7, r7, #1
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

/* 0xB0-0xB7: OR r/(HL) */
op_b0_or_b:
    ldrh    r1, [r6, #CPU_BC]
    lsrs    r1, r1, #8
    uxtb    r1, r1
    b       .Lor_r1_4
op_b1_or_c:
    ldrh    r1, [r6, #CPU_BC]
    uxtb    r1, r1
    b       .Lor_r1_4
op_b2_or_d:
    ldrh    r1, [r6, #CPU_DE]
    lsrs    r1, r1, #8
    uxtb    r1, r1
    b       .Lor_r1_4
op_b3_or_e:
    ldrh    r1, [r6, #CPU_DE]
    uxtb    r1, r1
    b       .Lor_r1_4
op_b4_or_h:
    ldrh    r1, [r6, #CPU_HL]
    lsrs    r1, r1, #8
    uxtb    r1, r1
    b       .Lor_r1_4
op_b5_or_l:
    ldrh    r1, [r6, #CPU_HL]
    uxtb    r1, r1
    b       .Lor_r1_4
op_b6_or_hl:
    ldrh    r5, [r6, #CPU_HL]
    uxth    r0, r5
    bl      RdZ80
    uxtb    r1, r0
    b       .Lor_r1_7
op_b7_or_a:
    ldrh    r5, [r6, #CPU_AF]
    lsrs    r1, r5, #8
    uxtb    r1, r1
    b       .Lor_r1_4

.Lor_common:
    ldrh    r5, [r6, #CPU_AF]
    lsrs    r0, r5, #8
    uxtb    r0, r0
    uxtb    r1, r1
    orr     r0, r0, r1
    push    {lr}
    bl      .Llogic_szp
    pop     {lr}
    lsls    r5, r0, #8
    orr     r5, r5, r12
    strh    r5, [r6, #CPU_AF]
    bx      lr

.Lor_r1_4:
    bl      .Lor_common
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

.Lor_r1_7:
    bl      .Lor_common
    add     r7, r7, #1
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

/* 0xA8-0xAF: XOR r/(HL) */
op_a8_xor_b:
    ldrh    r1, [r6, #CPU_BC]
    lsrs    r1, r1, #8
    uxtb    r1, r1
    b       .Lxor_r1_4
op_a9_xor_c:
    ldrh    r1, [r6, #CPU_BC]
    uxtb    r1, r1
    b       .Lxor_r1_4
op_aa_xor_d:
    ldrh    r1, [r6, #CPU_DE]
    lsrs    r1, r1, #8
    uxtb    r1, r1
    b       .Lxor_r1_4
op_ab_xor_e:
    ldrh    r1, [r6, #CPU_DE]
    uxtb    r1, r1
    b       .Lxor_r1_4
op_ac_xor_h:
    ldrh    r1, [r6, #CPU_HL]
    lsrs    r1, r1, #8
    uxtb    r1, r1
    b       .Lxor_r1_4
op_ad_xor_l:
    ldrh    r1, [r6, #CPU_HL]
    uxtb    r1, r1
    b       .Lxor_r1_4
op_ae_xor_hl:
    ldrh    r5, [r6, #CPU_HL]
    uxth    r0, r5
    bl      RdZ80
    uxtb    r1, r0
    b       .Lxor_r1_7
op_af_xor_a:
    ldrh    r5, [r6, #CPU_AF]
    lsrs    r1, r5, #8
    uxtb    r1, r1
    b       .Lxor_r1_4

.Lxor_common:
    ldrh    r5, [r6, #CPU_AF]
    lsrs    r0, r5, #8
    uxtb    r0, r0
    uxtb    r1, r1
    eor     r0, r0, r1
    push    {lr}
    bl      .Llogic_szp
    pop     {lr}
    lsls    r5, r0, #8
    orr     r5, r5, r12
    strh    r5, [r6, #CPU_AF]
    bx      lr

.Lxor_r1_4:
    bl      .Lxor_common
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

.Lxor_r1_7:
    bl      .Lxor_common
    add     r7, r7, #1
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

/* 0xB8-0xBF: CP r/(HL) (like SUB but does not store A) */
op_b8_cp_b:
    ldrh    r1, [r6, #CPU_BC]
    lsrs    r1, r1, #8
    uxtb    r1, r1
    b       .Lcp_r1_4
op_b9_cp_c:
    ldrh    r1, [r6, #CPU_BC]
    uxtb    r1, r1
    b       .Lcp_r1_4
op_ba_cp_d:
    ldrh    r1, [r6, #CPU_DE]
    lsrs    r1, r1, #8
    uxtb    r1, r1
    b       .Lcp_r1_4
op_bb_cp_e:
    ldrh    r1, [r6, #CPU_DE]
    uxtb    r1, r1
    b       .Lcp_r1_4
op_bc_cp_h:
    ldrh    r1, [r6, #CPU_HL]
    lsrs    r1, r1, #8
    uxtb    r1, r1
    b       .Lcp_r1_4
op_bd_cp_l:
    ldrh    r1, [r6, #CPU_HL]
    uxtb    r1, r1
    b       .Lcp_r1_4
op_be_cp_hl:
    ldrh    r5, [r6, #CPU_HL]
    uxth    r0, r5
    bl      RdZ80
    uxtb    r1, r0
    b       .Lcp_r1_7
op_bf_cp_a:
    ldrh    r5, [r6, #CPU_AF]
    lsrs    r1, r5, #8
    uxtb    r1, r1
    b       .Lcp_r1_4

.Lcp_common:
    /* r1 = operand */
    ldrh    r5, [r6, #CPU_AF]
    lsrs    r0, r5, #8         /* r0 = A */
    uxtb    r0, r0
    uxtb    r1, r1
    cmp     r0, r1
    ite     lo
    movlo   r12, #1            /* borrow -> C */
    movhs   r12, #0
    subs    r2, r0, r1         /* r2 = result (A-op) */
    uxtb    r2, r2
    orr     r12, r12, #F_N
    cmp     r2, #0
    it      eq
    orreq   r12, r12, #F_Z
    tst     r2, #0x80
    it      ne
    orrne   r12, r12, #F_S
    /* H */
    eor     r3, r0, r1
    eor     r3, r3, r2
    tst     r3, #0x10
    it      ne
    orrne   r12, r12, #F_H
    /* V: (A^op) & (A^res) & 0x80 */
    eor     r3, r0, r1
    eor     r1, r0, r2
    and     r3, r3, r1
    tst     r3, #0x80
    it      ne
    orrne   r12, r12, #F_V
    /* store only flags, keep A */
    lsls    r5, r0, #8
    orr     r5, r5, r12
    strh    r5, [r6, #CPU_AF]
    bx      lr

.Lcp_r1_4:
    bl      .Lcp_common
    add     r7, r7, #1
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

.Lcp_r1_7:
    bl      .Lcp_common
    add     r7, r7, #1
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

/* Immediate logic ops (7 cycles) */
op_e6_and_n:
    add     r0, r7, #1
    uxth    r0, r0
    bl      RdZ80
    uxtb    r1, r0
    bl      .Land_common
    add     r7, r7, #2
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

op_f6_or_n:
    add     r0, r7, #1
    uxth    r0, r0
    bl      RdZ80
    uxtb    r1, r0
    bl      .Lor_common
    add     r7, r7, #2
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

op_ee_xor_n:
    add     r0, r7, #1
    uxth    r0, r0
    bl      RdZ80
    uxtb    r1, r0
    bl      .Lxor_common
    add     r7, r7, #2
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

op_fe_cp_n:
    add     r0, r7, #1
    uxth    r0, r0
    bl      RdZ80
    uxtb    r1, r0
    bl      .Lcp_common
    add     r7, r7, #2
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

/* Immediate 8-bit ALU ops (7 cycles): fetch byte at PC+1 */
op_c6_add_a_n:
    add     r0, r7, #1
    uxth    r0, r0
    bl      RdZ80
    uxtb    r1, r0
    bl      .Ladd_a_r1_common
    add     r7, r7, #2
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

op_ce_adc_a_n:
    add     r0, r7, #1
    uxth    r0, r0
    bl      RdZ80
    uxtb    r1, r0
    bl      .Ladc_r1_common
    add     r7, r7, #2
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

op_d6_sub_n:
    add     r0, r7, #1
    uxth    r0, r0
    bl      RdZ80
    uxtb    r1, r0
    bl      .Lsub_r1_common
    add     r7, r7, #2
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

op_de_sbc_a_n:
    add     r0, r7, #1
    uxth    r0, r0
    bl      RdZ80
    uxtb    r1, r0
    bl      .Lsbc_r1_common
    add     r7, r7, #2
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

/* 0x18: JR d (12 cycles) */
op_18_jr:
    add     r0, r7, #1         /* displacement byte */
    uxth    r0, r0
    bl      RdZ80
    sxtb    r5, r0             /* sign-extend disp */
    add     r7, r7, #2         /* opcode + disp */
    add     r7, r7, r5
    uxth    r7, r7
    subs    r4, r4, #12
    bgt     .Lloop
    b       .Lreturn

/* 0x20: JR NZ,d (7/12 cycles) */
op_20_jr_nz:
    ldrh    r2, [r6, #CPU_AF]
    ands    r2, r2, #F_Z
    beq     .Ljr_nz_taken
    add     r7, r7, #2         /* skip disp */
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn
.Ljr_nz_taken:
    add     r0, r7, #1
    uxth    r0, r0
    bl      RdZ80
    sxtb    r5, r0
    add     r7, r7, #2
    add     r7, r7, r5
    uxth    r7, r7
    subs    r4, r4, #12
    bgt     .Lloop
    b       .Lreturn

/* 0x28: JR Z,d (7/12 cycles) */
op_28_jr_z:
    ldrh    r2, [r6, #CPU_AF]
    ands    r2, r2, #F_Z
    bne     .Ljr_z_taken
    add     r7, r7, #2
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn
.Ljr_z_taken:
    add     r0, r7, #1
    uxth    r0, r0
    bl      RdZ80
    sxtb    r5, r0
    add     r7, r7, #2
    add     r7, r7, r5
    uxth    r7, r7
    subs    r4, r4, #12
    bgt     .Lloop
    b       .Lreturn

/* 0x30: JR NC,d (7/12 cycles) */
op_30_jr_nc:
    ldrh    r2, [r6, #CPU_AF]
    ands    r2, r2, #F_C
    beq     .Ljr_nc_taken
    add     r7, r7, #2
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn
.Ljr_nc_taken:
    add     r0, r7, #1
    uxth    r0, r0
    bl      RdZ80
    sxtb    r5, r0
    add     r7, r7, #2
    add     r7, r7, r5
    uxth    r7, r7
    subs    r4, r4, #12
    bgt     .Lloop
    b       .Lreturn

/* 0x38: JR C,d (7/12 cycles) */
op_38_jr_c:
    ldrh    r2, [r6, #CPU_AF]
    ands    r2, r2, #F_C
    bne     .Ljr_c_taken
    add     r7, r7, #2
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn
.Ljr_c_taken:
    add     r0, r7, #1
    uxth    r0, r0
    bl      RdZ80
    sxtb    r5, r0
    add     r7, r7, #2
    add     r7, r7, r5
    uxth    r7, r7
    subs    r4, r4, #12
    bgt     .Lloop
    b       .Lreturn

/* 0xC2: JP NZ,nn (10 cycles) */
op_c2_jp_nz:
    ldrh    r2, [r6, #CPU_AF]
    ands    r2, r2, #F_Z
    beq     .Ljp_nz_taken
    add     r7, r7, #3
    subs    r4, r4, #10
    bgt     .Lloop
    b       .Lreturn
.Ljp_nz_taken:
    add     r0, r7, #1
    uxth    r0, r0
    bl      RdZ80
    uxtb    r5, r0
    add     r0, r7, #2
    uxth    r0, r0
    bl      RdZ80
    uxtb    r0, r0
    orr     r7, r5, r0, lsl #8
    uxth    r7, r7
    subs    r4, r4, #10
    bgt     .Lloop
    b       .Lreturn

/* 0xCA: JP Z,nn (10 cycles) */
op_ca_jp_z:
    ldrh    r2, [r6, #CPU_AF]
    ands    r2, r2, #F_Z
    bne     .Ljp_z_taken
    add     r7, r7, #3
    subs    r4, r4, #10
    bgt     .Lloop
    b       .Lreturn
.Ljp_z_taken:
    add     r0, r7, #1
    uxth    r0, r0
    bl      RdZ80
    uxtb    r5, r0
    add     r0, r7, #2
    uxth    r0, r0
    bl      RdZ80
    uxtb    r0, r0
    orr     r7, r5, r0, lsl #8
    uxth    r7, r7
    subs    r4, r4, #10
    bgt     .Lloop
    b       .Lreturn

/* 0xD2: JP NC,nn (10 cycles) */
op_d2_jp_nc:
    ldrh    r2, [r6, #CPU_AF]
    ands    r2, r2, #F_C
    beq     .Ljp_nc_taken
    add     r7, r7, #3
    subs    r4, r4, #10
    bgt     .Lloop
    b       .Lreturn
.Ljp_nc_taken:
    add     r0, r7, #1
    uxth    r0, r0
    bl      RdZ80
    uxtb    r5, r0
    add     r0, r7, #2
    uxth    r0, r0
    bl      RdZ80
    uxtb    r0, r0
    orr     r7, r5, r0, lsl #8
    uxth    r7, r7
    subs    r4, r4, #10
    bgt     .Lloop
    b       .Lreturn

/* 0xDA: JP C,nn (10 cycles) */
op_da_jp_c:
    ldrh    r2, [r6, #CPU_AF]
    ands    r2, r2, #F_C
    bne     .Ljp_c_taken
    add     r7, r7, #3
    subs    r4, r4, #10
    bgt     .Lloop
    b       .Lreturn
.Ljp_c_taken:
    add     r0, r7, #1
    uxth    r0, r0
    bl      RdZ80
    uxtb    r5, r0
    add     r0, r7, #2
    uxth    r0, r0
    bl      RdZ80
    uxtb    r0, r0
    orr     r7, r5, r0, lsl #8
    uxth    r7, r7
    subs    r4, r4, #10
    bgt     .Lloop
    b       .Lreturn

/* 0xE9: JP (HL) (4 cycles) */
op_e9_jp_hl:
    ldrh    r7, [r6, #CPU_HL]
    uxth    r7, r7
    add     r7, r7, #0        /* no fetch, just branch */
    subs    r4, r4, #4
    bgt     .Lloop
    b       .Lreturn

/* 0x77: LD (HL),A (7 cycles) */
op_77_ld_hl_a:
    /* load A */
    ldrh    r5, [r6, #CPU_AF]
    lsr     r5, r5, #8         /* r5 = A */
    /* HL already in memory; fetch */
    ldrh    r0, [r6, #CPU_HL]
    uxth    r0, r0
    mov     r1, r5             /* value */
    bl      WrZ80
    add     r7, r7, #1         /* opcode only */
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

/* 0x70: LD (HL),B (7 cycles) */
op_70_ld_hl_b:
    ldrh    r5, [r6, #CPU_BC]
    lsr     r5, r5, #8         /* r5 = B */
    uxtb    r5, r5
    ldrh    r0, [r6, #CPU_HL]
    uxth    r0, r0
    mov     r1, r5
    bl      WrZ80
    add     r7, r7, #1
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

/* 0x71: LD (HL),C (7 cycles) */
op_71_ld_hl_c:
    ldrh    r5, [r6, #CPU_BC]
    uxtb    r5, r5             /* r5 = C */
    ldrh    r0, [r6, #CPU_HL]
    uxth    r0, r0
    mov     r1, r5
    bl      WrZ80
    add     r7, r7, #1
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

/* 0x72: LD (HL),D (7 cycles) */
op_72_ld_hl_d:
    ldrh    r5, [r6, #CPU_DE]
    lsr     r5, r5, #8         /* r5 = D */
    uxtb    r5, r5
    ldrh    r0, [r6, #CPU_HL]
    uxth    r0, r0
    mov     r1, r5
    bl      WrZ80
    add     r7, r7, #1
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

/* 0x73: LD (HL),E (7 cycles) */
op_73_ld_hl_e:
    ldrh    r5, [r6, #CPU_DE]
    uxtb    r5, r5             /* r5 = E */
    ldrh    r0, [r6, #CPU_HL]
    uxth    r0, r0
    mov     r1, r5
    bl      WrZ80
    add     r7, r7, #1
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

/* 0x74: LD (HL),H (7 cycles) */
op_74_ld_hl_h:
    ldrh    r5, [r6, #CPU_HL]
    lsr     r5, r5, #8         /* r5 = H */
    uxtb    r5, r5
    ldrh    r0, [r6, #CPU_HL]
    uxth    r0, r0
    mov     r1, r5
    bl      WrZ80
    add     r7, r7, #1
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

/* 0x75: LD (HL),L (7 cycles) */
op_75_ld_hl_l:
    ldrh    r5, [r6, #CPU_HL]
    uxtb    r5, r5             /* r5 = L */
    ldrh    r0, [r6, #CPU_HL]
    uxth    r0, r0
    mov     r1, r5
    bl      WrZ80
    add     r7, r7, #1
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

/* 0x76: HALT (4 cycles) */
op_76_halt:
    /* set HALT flag and stop execution like C core */
    ldrb    r0, [r6, #CPU_IFF]
    orr     r0, r0, #IFF_HALT
    strb    r0, [r6, #CPU_IFF]
    movs    r0, #0
    str     r0, [r6, #CPU_IBACKUP]
    movs    r4, #0             /* terminate timeslice */
    b       .Lreturn

/* 0xC3: JP nn (10 cycles) */
op_c3_jp:
    add     r0, r7, #1         /* low */
    uxth    r0, r0
    bl      RdZ80
    uxtb    r5, r0
    add     r0, r7, #2         /* high */
    uxth    r0, r0
    bl      RdZ80
    uxtb    r0, r0
    orr     r7, r5, r0, lsl #8
    uxth    r7, r7
    subs    r4, r4, #10
    bgt     .Lloop
    b       .Lreturn

/* 0xC9: RET (10 cycles) */
op_c9_ret:
    /* Keep state in callee-saved regs: RdZ80 clobbers r0-r3 */
    ldrh    r5, [r6, #CPU_SP]
    uxth    r5, r5
    /* pop low */
    mov     r0, r5
    bl      RdZ80
    uxtb    r7, r0            /* r7 = low byte (temporary) */
    add     r5, r5, #1
    uxth    r5, r5
    /* pop high */
    mov     r0, r5
    bl      RdZ80
    uxtb    r0, r0
    orr     r7, r7, r0, lsl #8
    uxth    r7, r7
    add     r5, r5, #1
    uxth    r5, r5
    strh    r5, [r6, #CPU_SP]
    subs    r4, r4, #10
    bgt     .Lloop
    b       .Lreturn

/* 0xCD: CALL nn (17 cycles) */
op_cd_call:
    /* Push return address (PC+3) on stack; keep SP in r5 (callee-saved) */
    ldrh    r5, [r6, #CPU_SP]
    uxth    r5, r5

    /* SP-- ; write high(return) */
    subs    r5, r5, #1
    uxth    r5, r5
    mov     r0, r5
    add     r1, r7, #3
    uxth    r1, r1
    lsr     r1, r1, #8
    uxtb    r1, r1
    bl      WrZ80

    /* SP-- ; write low(return) */
    subs    r5, r5, #1
    uxth    r5, r5
    mov     r0, r5
    add     r1, r7, #3
    uxth    r1, r1
    uxtb    r1, r1
    bl      WrZ80

    strh    r5, [r6, #CPU_SP]

    /* Fetch target nn from PC+1/PC+2 and jump */
    add     r0, r7, #1
    uxth    r0, r0
    bl      RdZ80
    uxtb    r5, r0             /* low byte */
    add     r0, r7, #2
    uxth    r0, r0
    bl      RdZ80
    uxtb    r0, r0
    orr     r7, r5, r0, lsl #8
    uxth    r7, r7

    subs    r4, r4, #17
    bgt     .Lloop
    b       .Lreturn

/* RST helpers: each pushes PC+1 then jumps to vector (11 cycles) */
op_c7_rst_00:
    ldrh    r5, [r6, #CPU_SP]
    uxth    r5, r5
    subs    r5, r5, #1
    uxth    r5, r5
    mov     r0, r5
    add     r1, r7, #1
    uxth    r1, r1
    lsr     r1, r1, #8
    uxtb    r1, r1
    bl      WrZ80
    subs    r5, r5, #1
    uxth    r5, r5
    mov     r0, r5
    add     r1, r7, #1
    uxth    r1, r1
    uxtb    r1, r1
    bl      WrZ80
    strh    r5, [r6, #CPU_SP]
    movs    r7, #0x00
    subs    r4, r4, #11
    bgt     .Lloop
    b       .Lreturn

op_cf_rst_08:
    ldrh    r5, [r6, #CPU_SP]
    uxth    r5, r5
    subs    r5, r5, #1
    uxth    r5, r5
    mov     r0, r5
    add     r1, r7, #1
    uxth    r1, r1
    lsr     r1, r1, #8
    uxtb    r1, r1
    bl      WrZ80
    subs    r5, r5, #1
    uxth    r5, r5
    mov     r0, r5
    add     r1, r7, #1
    uxth    r1, r1
    uxtb    r1, r1
    bl      WrZ80
    strh    r5, [r6, #CPU_SP]
    movs    r7, #0x08
    subs    r4, r4, #11
    bgt     .Lloop
    b       .Lreturn

op_d7_rst_10:
    ldrh    r5, [r6, #CPU_SP]
    uxth    r5, r5
    subs    r5, r5, #1
    uxth    r5, r5
    mov     r0, r5
    add     r1, r7, #1
    uxth    r1, r1
    lsr     r1, r1, #8
    uxtb    r1, r1
    bl      WrZ80
    subs    r5, r5, #1
    uxth    r5, r5
    mov     r0, r5
    add     r1, r7, #1
    uxth    r1, r1
    uxtb    r1, r1
    bl      WrZ80
    strh    r5, [r6, #CPU_SP]
    movs    r7, #0x10
    subs    r4, r4, #11
    bgt     .Lloop
    b       .Lreturn

op_df_rst_18:
    ldrh    r5, [r6, #CPU_SP]
    uxth    r5, r5
    subs    r5, r5, #1
    uxth    r5, r5
    mov     r0, r5
    add     r1, r7, #1
    uxth    r1, r1
    lsr     r1, r1, #8
    uxtb    r1, r1
    bl      WrZ80
    subs    r5, r5, #1
    uxth    r5, r5
    mov     r0, r5
    add     r1, r7, #1
    uxth    r1, r1
    uxtb    r1, r1
    bl      WrZ80
    strh    r5, [r6, #CPU_SP]
    movs    r7, #0x18
    subs    r4, r4, #11
    bgt     .Lloop
    b       .Lreturn

op_e7_rst_20:
    ldrh    r5, [r6, #CPU_SP]
    uxth    r5, r5
    subs    r5, r5, #1
    uxth    r5, r5
    mov     r0, r5
    add     r1, r7, #1
    uxth    r1, r1
    lsr     r1, r1, #8
    uxtb    r1, r1
    bl      WrZ80
    subs    r5, r5, #1
    uxth    r5, r5
    mov     r0, r5
    add     r1, r7, #1
    uxth    r1, r1
    uxtb    r1, r1
    bl      WrZ80
    strh    r5, [r6, #CPU_SP]
    movs    r7, #0x20
    subs    r4, r4, #11
    bgt     .Lloop
    b       .Lreturn

op_ef_rst_28:
    ldrh    r5, [r6, #CPU_SP]
    uxth    r5, r5
    subs    r5, r5, #1
    uxth    r5, r5
    mov     r0, r5
    add     r1, r7, #1
    uxth    r1, r1
    lsr     r1, r1, #8
    uxtb    r1, r1
    bl      WrZ80
    subs    r5, r5, #1
    uxth    r5, r5
    mov     r0, r5
    add     r1, r7, #1
    uxth    r1, r1
    uxtb    r1, r1
    bl      WrZ80
    strh    r5, [r6, #CPU_SP]
    movs    r7, #0x28
    subs    r4, r4, #11
    bgt     .Lloop
    b       .Lreturn

op_f7_rst_30:
    ldrh    r5, [r6, #CPU_SP]
    uxth    r5, r5
    subs    r5, r5, #1
    uxth    r5, r5
    mov     r0, r5
    add     r1, r7, #1
    uxth    r1, r1
    lsr     r1, r1, #8
    uxtb    r1, r1
    bl      WrZ80
    subs    r5, r5, #1
    uxth    r5, r5
    mov     r0, r5
    add     r1, r7, #1
    uxth    r1, r1
    uxtb    r1, r1
    bl      WrZ80
    strh    r5, [r6, #CPU_SP]
    movs    r7, #0x30
    subs    r4, r4, #11
    bgt     .Lloop
    b       .Lreturn

op_ff_rst_38:
    ldrh    r5, [r6, #CPU_SP]
    uxth    r5, r5
    subs    r5, r5, #1
    uxth    r5, r5
    mov     r0, r5
    add     r1, r7, #1
    uxth    r1, r1
    lsr     r1, r1, #8
    uxtb    r1, r1
    bl      WrZ80
    subs    r5, r5, #1
    uxth    r5, r5
    mov     r0, r5
    add     r1, r7, #1
    uxth    r1, r1
    uxtb    r1, r1
    bl      WrZ80
    strh    r5, [r6, #CPU_SP]
    movs    r7, #0x38
    subs    r4, r4, #11
    bgt     .Lloop
    b       .Lreturn

/* 0x7E: LD A,(HL) (7 cycles) */
op_7e_ld_a_hl:
    ldrh    r0, [r6, #CPU_HL]
    uxth    r0, r0
    bl      RdZ80
    /* store into A */
    ldrh    r5, [r6, #CPU_AF]
    bfi     r5, r0, #8, #8
    strh    r5, [r6, #CPU_AF]
    add     r7, r7, #1         /* opcode only */
    subs    r4, r4, #7
    bgt     .Lloop
    b       .Lreturn

.Lreturn:
    /* Write back PC */
    strh    r7, [r6, #CPU_PC]
    mov     r0, r4
    pop     {r4-r7, pc}

.Lfallback:
    /* Store PC (not advanced) so C re-executes opcode */
    strh    r7, [r6, #CPU_PC]
    mov     r0, r6
    mov     r1, r4
    bl      ExecZ80
    mov     r4, r0
    pop     {r4-r7, pc}

#endif /* Z80_ARM_ENABLE */

.size z80_arm_exec, .-z80_arm_exec
