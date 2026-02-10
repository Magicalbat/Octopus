#include "base/base.h"
#include "platform/platform.h"

#include "base/base.c"
#include "platform/platform.c"

int main(int argc, char** argv) {
    UNUSED(argc);
    UNUSED(argv);

    log_frame_begin();

    plat_init();

    u64 seeds[2] = { 0 };
    plat_get_entropy(seeds, sizeof(seeds));
    prng_seed(seeds[0], seeds[1]);

    mem_arena* perm_arena = arena_create(MiB(64), KiB(264), true);

    // End of setup error frame
    {
        string8 err_str = log_frame_end(
            perm_arena, LOG_WARN | LOG_ERROR, LOG_RES_CONCAT, true
        );

        if (err_str.size) {
            printf("\x1b[31m%.*s\x1b[0m\n", (int)err_str.size, err_str.str);
            return 1;
        }
    }

    arena_destroy(perm_arena);

    return 0;
}

