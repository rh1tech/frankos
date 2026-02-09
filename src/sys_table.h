#ifndef SYS_TABLE_H
#define SYS_TABLE_H

#include <pico.h>

// weak_alias used by musl source files
#ifndef weak_alias
#define weak_alias(old, new) \
	extern __typeof(old) new __attribute__((__weak__, __alias__(#old)))
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define __in_systable(group) __attribute__((section(".sys_table" group)))
#define __in_lfa(group) __attribute__((section(".low_flash" group)))
#define __in_hfa(group) __attribute__((section(".high_flash" group)))
// TODO: if not enough HFA
#define __libc(group) __attribute__((section(".libc_flash" group)))
//#define __libc(group) __attribute__((section(".high_flash" group)))

extern unsigned long __in_systable() __aligned(4096) sys_table_ptrs[];

/*
| Секция        | ORIGIN     |          END | Размер (КБ) | Атрибуты | Комментарий                     |
| ------------- | ---------- | -----------: | ----------: | -------: | ------------------------------- |
| **LFA**       | 0x10000000 |   0x1000FFFF |        64.0 |       rx | LOW FLASH AREA (boot-loader)    |
| **PFA**       | 0x10010000 |   0x10DB9FFF |    13 992.0*|       rx | PROGRAM FLASH AREA (paged apps) |
| **LCA**       | 0x10E00000 |   0x10FDFFFF |     1 920.0 |       rw | LIBC FLASH AREA                 |
| **HFA**       | 0x10FE0000 | 0x10FFEEFF?* |       124.0 |       rw | HIGH FLASH AREA                 |
| **SYS_TABLE** | 0x10FFF000 |   0x10FFFFFF |         4.0 |       rw | таблица указателей              |
| **PSRAM**     | 0x11000000 |   0x11FFFFFF |    16 384.0*|      rwx | shared PSRAM (16M)              |
| **RAM**       | 0x20000000 |   0x2007FFFF |       512.0 |      rwx | shared fast SRAM                |
| **SCRATCH_X** | 0x20080000 |   0x20080FFF |         4.0 |      rwx | shared stack X                  |
| **SCRATCH_Y** | 0x20081000 |   0x20081FFF |         4.0 |      rwx | technical stack Y               |

* May be less, so - to be detected in runtime.
*/

#ifdef __cplusplus
}
#endif

#endif // SYS_TABLE_H
