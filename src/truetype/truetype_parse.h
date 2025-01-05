#ifndef TRUETYPE_PARSE_H
#define TRUETYPE_PARSE_H

#include "base/base.h"

typedef struct {
    u32 offset;
    u32 length;
} tt_table_info;

typedef struct {
    string8 file;

    u16 num_glyphs;
    // Max of single and composite glyphs
    u16 max_glyph_points;

    struct {
        tt_table_info loca;
        tt_table_info glyf;
        tt_table_info hhea;
        tt_table_info hmtx;
    } tables;

    u64 cmap_offset;

    u16 loca_format;
} tt_font_info;

b32 tt_init_font(string8 file, tt_font_info* font_info);

#endif // TRUETYPE_PARSE_H

