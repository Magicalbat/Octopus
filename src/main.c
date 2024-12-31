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

    gfx_window* win = gfx_win_create(perm_arena, 1280, 720, STR8_LIT("Test Window"));

    gfx_win_make_current(win);

    gfx_win_clear_color(win, (vec4f){ 0.0f, 0.5f, 0.5f, 1.0f });
    while (!win->should_close) {
        gfx_win_process_events(win);

        gfx_win_clear(win);

        gfx_win_swap_buffers(win);
    }

    gfx_win_destroy(win);

    arena_destroy(perm_arena);

    return 0;
}

