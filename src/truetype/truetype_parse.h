
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

    u16 num_hmetrics;

    struct {
        tt_table_info head;
        tt_table_info hhea;
        tt_table_info loca;
        tt_table_info glyf;
        tt_table_info hmtx;
    } tables;

    // Offset from start of file, not from start of cmap table
    u64 cmap_subtable_offset;
    // 4 or 12
    u16 cmap_format;

    u16 loca_format;
} tt_font_info;

// Font bounding box
// (x_min, y_min) is the lower left corner
// (x_max, y_max) is the upper right corner
typedef struct {
    f32 x_min;
    f32 y_min;
    f32 x_max;
    f32 y_max;
} tt_bounding_box;

typedef struct {
    f32 advance_width;
    f32 left_side_bearing;
} tt_h_metrics;

void tt_init_font(string8 file, tt_font_info* font_info);

// Returns pixels / (ascent - descent)
f32 tt_get_scale_for_height(const tt_font_info* font_info, f32 pixels);

// Returns pixels / units_per_em
f32 tt_get_scale_for_em(const tt_font_info* font_info, f32 pixels);

u32 tt_get_glyph_index(const tt_font_info* font_info, u32 codepoint);

tt_bounding_box tt_get_glyph_box(const tt_font_info* font_info, u32 glyph_index);

tt_h_metrics tt_get_glyph_h_metrics(const tt_font_info* font_info, u32 glyph_index);

// segments should be pre-allocated with font_info.max_glyph_points segments
// offset is applied after transform
// Returns the number of segments written
u32 tt_get_glyph_outline(
    const tt_font_info* font_info,
    u32 glyph_index,
    tt_segment* segments,
    u32 segments_offset,
    mat2f transform,
    vec2f offset
);

