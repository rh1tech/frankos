#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "hardware/vreg.h"

/*
 * Board Configuration for Rhea OS
 *
 * BOARD_M2 - M2 GPIO layout (default)
 *
 * M2 GPIO Layout:
 *   HDMI: CLKN=12, CLKP=13, D0N=14, D0P=15, D1N=16, D1P=17, D2N=18, D2P=19
 *   PS/2: CLK=2, DATA=3
 */

#if !defined(BOARD_M1) && !defined(BOARD_M2)
#define BOARD_M2
#endif

//=============================================================================
// CPU Speed Defaults
//=============================================================================
#ifndef CPU_CLOCK_MHZ
#define CPU_CLOCK_MHZ 252
#endif

#ifndef CPU_VOLTAGE
#define CPU_VOLTAGE VREG_VOLTAGE_1_50
#endif

//=============================================================================
// M1 Layout Configuration
//=============================================================================
#ifdef BOARD_M1

#define HDMI_PIN_CLKN 6
#define HDMI_PIN_CLKP 7
#define HDMI_PIN_D0N  8
#define HDMI_PIN_D0P  9
#define HDMI_PIN_D1N  10
#define HDMI_PIN_D1P  11
#define HDMI_PIN_D2N  12
#define HDMI_PIN_D2P  13

#define HDMI_BASE_PIN HDMI_PIN_CLKN

#define PS2_PIN_CLK  0
#define PS2_PIN_DATA 1

#endif // BOARD_M1

//=============================================================================
// M2 Layout Configuration
//=============================================================================
#ifdef BOARD_M2

#define HDMI_PIN_CLKN 12
#define HDMI_PIN_CLKP 13
#define HDMI_PIN_D0N  14
#define HDMI_PIN_D0P  15
#define HDMI_PIN_D1N  16
#define HDMI_PIN_D1P  17
#define HDMI_PIN_D2N  18
#define HDMI_PIN_D2P  19

#define HDMI_BASE_PIN HDMI_PIN_CLKN

#define PS2_PIN_CLK  2
#define PS2_PIN_DATA 3

#endif // BOARD_M2

//=============================================================================
// PIO Assignments
//=============================================================================
#ifndef PIO_VIDEO
#define PIO_VIDEO pio1
#endif
#ifndef PIO_VIDEO_ADDR
#define PIO_VIDEO_ADDR pio1
#endif

#endif // BOARD_CONFIG_H
