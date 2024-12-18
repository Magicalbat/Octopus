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

    u64 seeds[2] = { 0 };
    plat_get_entropy(seeds, sizeof(seeds));
    prng_seed(seeds[0], seeds[1]);

    mem_arena* perm_arena = arena_create(MiB(64), KiB(264));

    for (u32 i = 0; i < 64; i++) {
        printf("%u\n", prng_rand() & 0xf);
    }

    arena_destroy(perm_arena);

    return 0;

#endif
}

