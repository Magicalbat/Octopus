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
    srand(plat_time_usec());

    mem_arena* perm_arena = arena_create(MiB(64), KiB(264));

    string8 strings[10] = {
        STR8_LIT("String 0\n"),
        STR8_LIT("String String 1\n"),
        STR8_LIT("String String String 2\n"),
        STR8_LIT("String String String String 3\n"),
        STR8_LIT("String String String String String 4\n"),
        STR8_LIT("String String String String String String 5\n"),
        STR8_LIT("String String String String String String String 6\n"),
        STR8_LIT("String String String String String String String String 7\n"),
        STR8_LIT("String String String String String String String String String 8\n"),
        STR8_LIT("String String String String String String String String String String 9\n"),
    };

    for (u32 i = 0; i < 10; i++) {
        mem_arena_temp scratch = arena_scratch_get(NULL, 0);

        u64 start = plat_time_usec();

        string8_list output = { 0 };

        for (u32 i = 0; i < 1 << 16; i++) {
            str8_list_push(scratch.arena, &output, strings[rand() % 10]);
        }

        plat_file_write(STR8_LIT("test.txt"), &output, false);

        u64 end = plat_time_usec();

        printf("%f ms, %lu bytes\n", (f32)(end - start) * 1e-3, arena_get_pos(scratch.arena));

        arena_scratch_release(scratch);
    }

    arena_destroy(perm_arena);

    return 0;

#endif
}

