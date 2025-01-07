#include <stdio.h>
#include <stdlib.h>

#include "base/base.h"
#include "platform/platform.h"

#include "gfx/gfx.h"
#include "gfx/opengl/opengl_defs.h"

#include "truetype/truetype.h"

int main(int argc, char** argv) {
    plat_init();

    u64 seeds[2] = { 0 };
    plat_get_entropy(seeds, sizeof(seeds));
    prng_seed(seeds[0], seeds[1]);

    mem_arena* perm_arena = arena_create(MiB(64), KiB(264));

    char* font_path = argc > 1 ? argv[1] : "res/Hack.ttf";
    string8 font_file = plat_file_read(perm_arena, str8_from_cstr((u8*)font_path));
    tt_font_info font = { 0 };

    b32 ret = tt_init_font(font_file, &font);

    if (!ret) {
        printf("Failed to load font file\n");
    }

    printf(
        "num_glyphs: %u\n"
        "max_glyph_points: %u\n\n"
        "loca: { %u %u }\n"
        "glyf: { %u %u }\n"
        "hhea: { %u %u }\n"
        "hmtx: { %u %u }\n\n"
        "cmap_subtable_offset: %llu\n"
        "cmap_format: %u\n\n"
        "loca_format: %u\n"
        "max_glyph_index: %u\n",
        font.num_glyphs,
        font.max_glyph_points,
        font.tables.loca.offset, font.tables.loca.length,
        font.tables.glyf.offset, font.tables.glyf.length,
        font.tables.hhea.offset, font.tables.hhea.length,
        font.tables.hmtx.offset, font.tables.hmtx.length,
        font.cmap_subtable_offset,
        font.cmap_format,
        font.loca_format,
        font.max_glyph_index
    );

    /*gfx_window* win = gfx_win_create(perm_arena, 1280, 720, STR8_LIT("Test Window"));

    gfx_win_make_current(win);

    gfx_win_clear_color(win, (vec4f){ 0.0f, 0.5f, 0.5f, 1.0f });
    while (!win->should_close) {
        gfx_win_process_events(win);

        gfx_win_clear(win);

        gfx_win_swap_buffers(win);
    }

    gfx_win_destroy(win);*/

    arena_destroy(perm_arena);

    return 0;
}

