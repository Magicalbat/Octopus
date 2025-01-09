#ifndef TRUETYPE_PARSE_H
#define TRUETYPE_PARSE_H

#include "base/base.h"

typedef struct {
    u32 offset;
    u32 length;
} tt_table_info;

typedef struct {
    b32 initialized;

    u16 num_glyphs;
    // Max of single and composite glyphs
    u16 max_glyph_points;

    struct {
        tt_table_info loca;
        tt_table_info glyf;
        tt_table_info hhea;
        tt_table_info hmtx;
    } tables;

    // Offset from start of file, not from start of cmap table
    u64 cmap_subtable_offset;
    // 4 or 12
    u16 cmap_format;

    u16 loca_format;
    u32 max_glyph_index;
} tt_font_info;

void tt_init_font(string8 file, tt_font_info* font_info);
u32 tt_get_glyph_index(string8 file, const tt_font_info* font_info, u32 codepoint);

#endif // TRUETYPE_PARSE_H

