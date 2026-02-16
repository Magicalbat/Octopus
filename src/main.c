
#include "base/base.h"
#include "platform/platform.h"
#include "win/win.h"

#include "base/base.c"
#include "platform/platform.c"
#include "win/win.c"

int main(int argc, char** argv) {
    UNUSED(argc);
    UNUSED(argv);

    log_frame_begin();

    plat_init();

    u64 seeds[2] = { 0 };
    plat_get_entropy(seeds, sizeof(seeds));
    prng_seed(seeds[0], seeds[1]);

    mem_arena* perm_arena = arena_create(MiB(64), KiB(264), true);

    window* win = win_create(perm_arena, 1280, 720, STR8_LIT("Octopus"));

    // End of setup error frame
    {
        mem_arena_temp scratch = arena_scratch_get(NULL, 0);
        string8 other_logs = log_frame_peek(
            scratch.arena, LOG_INFO | LOG_WARN, LOG_RES_CONCAT, true
        );

        if (other_logs.size) {
            printf("%.*s\n", STR8_FMT(other_logs));
        }

        arena_scratch_release(scratch);

        string8 err_str = log_frame_end(
            perm_arena, LOG_ERROR, LOG_RES_CONCAT, true
        );

        if (err_str.size) {
            printf("\x1b[31m%.*s\x1b[0m\n", STR8_FMT(err_str));
            return 1;
        }
    }

    while ((win->flags & WIN_FLAG_SHOULD_CLOSE) == 0) {
        win_process_events(win);

        if (win->mouse_scroll.x || win->mouse_scroll.y) {
            printf("Scroll: %.2f %.2f\n", win->mouse_scroll.x, win->mouse_scroll.y);
        }
    }

    win_destroy(win);

    arena_destroy(perm_arena);

    return 0;
}

