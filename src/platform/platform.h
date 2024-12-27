#ifndef PLATFORM_H
#define PLATFORM_H

#include "base/base.h"

void plat_init(void);

// Do not modify the string 
string8 plat_get_name(void);

u64 plat_time_usec(void);

u64 plat_file_size(string8 file_name);
string8 plat_file_read(mem_arena* arena, string8 file_name);
b32 plat_file_write(string8 file_name, const string8_list* list, b32 append);
b32 plat_file_delete(string8 file_name);

void plat_get_entropy(void* data, u64 size);

// returns NULL on failure
void* plat_mem_reserve(u64 size);
b32 plat_mem_commit(void* mem, u64 size);
void plat_mem_decommit(void* mem, u64 size);
void plat_mem_release(void* mem, u64 size);

u32 plat_mem_page_size(void);

#endif // PLATFORM_H

