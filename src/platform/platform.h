#ifndef PLATFORM_H
#define PLATFORM_H

#include "base/base.h"

// returns NULL on failure
void* plat_mem_reserve(u64 size);
b32 plat_mem_commit(void* mem, u64 size);
void plat_mem_decommit(void* mem, u64 size);
void plat_mem_release(void* mem, u64 size);

u32 plat_mem_page_size(void);

#endif // PLATFORM_H

