#include <stdio.h>
#include <stdlib.h>

#include "base/base.h"
#include "platform/platform.h"
#include "profile/profile.h"

int main(int argc, char** argv) {
#ifdef PROFILE_MODE
    return profile_main(argc, argv);
#else

    UNUSED(argc);
    UNUSED(argv);

    plat_init();

    mem_arena* perm_arena = arena_create(MiB(64), KiB(264));

    u32 size = 1 << 19;
    u32* A = ARENA_PUSH_ARRAY(perm_arena, u32, size);
    u32* B = ARENA_PUSH_ARRAY(perm_arena, u32, size);
    volatile u32* C = ARENA_PUSH_ARRAY(perm_arena, u32, size);

    srand(plat_time_usec());
    for (u32 i = 0; i < size; i++) {
        A[i] = rand();
        B[i] = rand();
    }

    u64 start = plat_time_usec();
    for (u32 i = 0; i < size; i++) {
        C[i] = MIN(A[i], B[i]);
    }
    u64 end = plat_time_usec();

    printf("%lu\n", end-start);

    arena_destroy(perm_arena);

    return 0;

#endif
}

