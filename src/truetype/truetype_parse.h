
typedef struct {
    u32 offset;
    u32 length;
} tt_font_table;

typedef struct {
    b8 initialized;

    b8 cmap_sorted;

    i16 loca_format;
    u16 num_glyphs;
    u16 cmap_format;
    // Offset of selected cmap subtable, not cmap table itself
    // Offset is from beginning of file
    u32 cmap_offset;

    u32 max_glyph_points;
    u32 max_glyph_contours;

    tt_font_table head, glyf, hmtx, loca;
} tt_font_info;

void tt_font_init(string8 file, tt_font_info* info);

f32 tt_scale_for_em(string8 file, tt_font_info* info, f32 pixels_per_em);

tt_glyph_data tt_glyph_data_from_index(
    mem_arena* arena, string8 file,
    tt_font_info* info, u32 glyph_index
);

tt_glyph_data tt_glyph_data_from_codepoint(
    mem_arena* arena, string8 file,
    tt_font_info* info, u32 codepoint
);

u32 tt_glyph_index(string8 file, tt_font_info* info, u32 codepoint);

