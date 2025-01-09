#include <stdio.h>
#include <stdlib.h>

#include "base/base.h"
#include "platform/platform.h"

#include "gfx/gfx.h"
#include "gfx/opengl/opengl_defs.h"

#include "truetype/truetype.h"

int main(int argc, char** argv) {
    error_frame_begin();

    plat_init();

    u64 seeds[2] = { 0 };
    plat_get_entropy(seeds, sizeof(seeds));
    prng_seed(seeds[0], seeds[1]);

    mem_arena* perm_arena = arena_create(MiB(64), KiB(264), true);

    char* font_path = argc > 1 ? argv[1] : "res/Envy Code R.ttf";
    string8 font_file = plat_file_read(perm_arena, str8_from_cstr((u8*)font_path));
    tt_font_info font = { 0 };

    tt_init_font(font_file, &font);

    f32 scale = tt_get_scale(&font, 24.0f);
    tt_segment* segments = ARENA_PUSH_ARRAY(perm_arena, tt_segment, font.max_glyph_points);
    u32 glyph_index = tt_get_glyph_index(&font, 'A');
    u32 num_segments = tt_get_glyph_outline(&font, glyph_index, segments, scale);

    string8 err_str = error_frame_end(perm_arena, ERROR_OUTPUT_CONCAT);
    if (err_str.size) {
        printf("\x1b[31mError(s): %.*s\x1b[0m\n", (int)err_str.size, err_str.str);
        return 1;
    }

    /*printf(
        "num_glyphs: %u\n"
        "max_glyph_points: %u\n\n"
        "loca: { %u %u }\n"
        "glyf: { %u %u }\n"
        "hhea: { %u %u }\n"
        "hmtx: { %u %u }\n\n"
        "cmap_subtable_offset: %u\n"
        "cmap_format: %u\n\n"
        "loca_format: %u\n",
        font.num_glyphs,
        font.max_glyph_points,
        font.tables.loca.offset, font.tables.loca.length,
        font.tables.glyf.offset, font.tables.glyf.length,
        font.tables.hhea.offset, font.tables.hhea.length,
        font.tables.hmtx.offset, font.tables.hmtx.length,
        (u32)font.cmap_subtable_offset,
        font.cmap_format,
        font.loca_format
    );*/

    arena_destroy(perm_arena);

    return 0;
}

