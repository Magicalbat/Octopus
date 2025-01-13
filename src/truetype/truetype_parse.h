#ifndef TRUETYPE_PARSE_H
#define TRUETYPE_PARSE_H

#include "base/base_defs.h"
#include "base/base_str.h"

#include "truetype_segment.h"

typedef struct {
    u32 offset;
    u32 length;
} tt_table_info;

typedef struct {
    b32 initialized;

    string8 file;

    u16 num_glyphs;
    // Max of single and composite glyphs
    u16 max_glyph_points;
    // Maximum components referenced at top level of composite glyph
    u16 max_component_elements;

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
} tt_font_info;

void tt_init_font(string8 file, tt_font_info* font_info);
u32 tt_get_glyph_index(const tt_font_info* font_info, u32 codepoint);
f32 tt_get_scale(const tt_font_info* font_info, f32 height);
// segments should be pre-allocated with size font_info.max_glyph_points * sizeof(tt_segment)
// offset is applied after transform
// Returns the number of segments written
u32 tt_get_glyph_outline(const tt_font_info* font_info, u32 glyph_index, tt_segment* segments, u32 segments_offset, mat2f transform, vec2f offset);

#endif // TRUETYPE_PARSE_H

