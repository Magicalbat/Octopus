#include <stdio.h>
#include <stdlib.h>

#include "base/base.h"
#include "platform/platform.h"

#include "gfx/gfx.h"
#include "gfx/opengl/opengl_defs.h"

int main(int argc, char** argv) {
    UNUSED(argc);
    UNUSED(argv);

    plat_init();

    u64 seeds[2] = { 0 };
    plat_get_entropy(seeds, sizeof(seeds));
    prng_seed(seeds[0], seeds[1]);

    mem_arena* perm_arena = arena_create(MiB(64), KiB(264));

    string8 file_name = STR8_LIT("tmpüü.txt");

    string8_list strs = { 0 };
    for (u32 i = 0; i < 1000; i++) {
        str8_list_push(perm_arena, &strs, str8_createf(perm_arena, "Line %u\n", i));
    }

    plat_file_write(file_name, &strs, argc < 3);

    printf("%llu\n", plat_file_size(file_name));

    /*gfx_window* win = gfx_win_create(perm_arena, 1280, 720, STR8_LIT("Test Window"));

    glClearColor(0, 0.5, 0.5, 1);
    while (!win->should_close) {
        gfx_win_process_events(win);

        gfx_win_clear(win);

        gfx_win_swap_buffers(win);
    }

    gfx_win_destroy(win);*/

    arena_destroy(perm_arena);

    return 0;
}

