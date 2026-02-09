/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _ELF_H
#define _ELF_H

#include <stdint.h>
#include "ff.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ELF_MAGIC 0x464c457fu

#define EM_ARM 0x28u

#define EF_ARM_ABI_FLOAT_HARD 0x00000400u

#define PT_LOAD 0x00000001u

#pragma pack(push, 1)
struct elf_header {
    uint32_t    magic;
    uint8_t     arch_class;
    uint8_t     endianness;
    uint8_t     version;
    uint8_t     abi;
    uint8_t     abi_version;
    uint8_t     _pad[7];
    uint16_t    type;
    uint16_t    machine;
    uint32_t    version2;
};

typedef struct elf32_header {
    struct elf_header common;
    uint32_t    entry;
    uint32_t    ph_offset;
    uint32_t    sh_offset;
    uint32_t    flags;
    uint16_t    eh_size;
    uint16_t    ph_entry_size;
    uint16_t    ph_num;
    uint16_t    sh_entry_size;
    uint16_t    sh_num;
    uint16_t    sh_str_index;
} elf32_header;

struct elf32_ph_entry {
    uint32_t type;
    uint32_t offset;
    uint32_t vaddr;
    uint32_t paddr;
    uint32_t filez;
    uint32_t memsz;
    uint32_t flags;
    uint32_t align;
};

typedef struct {
  uint32_t	sh_name;
  uint32_t	sh_type;
  uint32_t	sh_flags;
  uint32_t	sh_addr;
  uint32_t	sh_offset;
  uint32_t	sh_size;
  uint32_t	sh_link;
  uint32_t	sh_info;
  uint32_t	sh_addralign;
  uint32_t	sh_entsize;
} elf32_shdr;

typedef struct {
  uint32_t	st_name;
  uint32_t	st_value;
  uint32_t	st_size;
  unsigned char	st_info;
  unsigned char	st_other;
  uint16_t	st_shndx;
} elf32_sym;

typedef struct {
  uint32_t rel_offset;
  uint32_t rel_info;
} elf32_rel;

typedef struct {
  uint32_t rela_offset;
  uint32_t rela_info;
  int32_t rela_addend;
} elf32_rela;

#pragma pack(pop)

#define STR_TAB_FUNC 2

#define STR_TAB_GLOBAL 1
#define STR_TAB_WEAK 2

#define STR_TAB_GLOBAL_FUNC ((STR_TAB_GLOBAL << 4) | STR_TAB_FUNC)
#define STR_TAB_WEAK_FUNC ((STR_TAB_WEAK << 4) | STR_TAB_FUNC)

#define REL_SEC 9

#define SHT_NULL       0
#define SHT_PROGBITS   1
#define SHT_SYMTAB     2
#define SHT_STRTAB     3
#define SHT_RELA       4
#define SHT_HASH       5
#define SHT_DYNAMIC    6
#define SHT_NOTE       7
#define SHT_NOBITS     8
#define SHT_REL        9
#define SHT_SHLIB      10
#define SHT_DYNSYM     11

#define SHF_WRITE     0x1  // section is wriable (do not save it into flash)
#define SHF_ALLOC     0x2  // section should be allocated
#define SHF_EXECINSTR 0x4  // section contains executables

void elfinfo(FIL *f, char *fn);

#ifdef __cplusplus
}
#endif


#endif
