#pragma once
#include <stdbool.h>
#include <inttypes.h>

uint32_t init_vram(char*);
uint32_t swap_size();
uint32_t swap_base_size();
uint8_t* swap_base();
uint32_t swap_page_size();
uint32_t swap_pages();
uint16_t* swap_pages_base();

uint8_t ram_page_read(uint32_t addr32);
uint16_t ram_page_read16(uint32_t addr32);
uint32_t ram_page_read32(uint32_t addr32);

void ram_page_write(uint32_t addr32, uint8_t value);
void ram_page_write16(uint32_t addr32, uint16_t value);
void ram_page_write32(uint32_t addr32, uint32_t value);

void read_vram_block(char* dst, uint32_t file_offset, uint32_t sz);
void flush_vram_block(const char* src, uint32_t file_offset, uint32_t sz);
