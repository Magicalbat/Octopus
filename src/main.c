#include <stdio.h>
#include <string.h>

#include "base/base.h"
#include "platform/platform.h"

int main(void) {
    mem_arena* perm_arena = arena_create(MiB(1), KiB(64));

    u8* buffer_a = (u8*)arena_push(perm_arena, KiB(900));
    u8* buffer_b = (u8*)arena_push(perm_arena, MiB(5));
    u64 pos = arena_get_pos(perm_arena);
    u8* buffer_c = (u8*)arena_push(perm_arena, MiB(10));
    arena_pop_to(perm_arena, pos);
    u8* buffer_d = (u8*)arena_push(perm_arena, KiB(10));
    buffer_d = (u8*)arena_push(perm_arena, KiB(1));

    arena_destroy(perm_arena);

    return 0;
}

