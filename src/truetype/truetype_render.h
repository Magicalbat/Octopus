#define TT_CORNER_DOT_THRESHOLD 0.95f

typedef enum {
    // Assumes a single channel 8-bit texture
    TT_RENDER_SDF,
    // Assumes a three channel 8-bit texture
    TT_RENDER_MSDF,
    // Assumes a four channel 8-bit texture
    TT_RENDER_TMSDF,

    TT_RENDER_COUNT
} tt_render_mode;

typedef struct {
    rectf bitmap_rect;
    vec2f offset;
    f32 x_advance;
} tt_render_glyph_info;

typedef struct {
    u8* data;

    u32 width;
    u32 height;
} tt_bitmap;

typedef struct {
    u8* data;
    u32 total_width;
    u32 total_height;

    u32 offset_x;
    u32 offset_y;
    u32 local_width;
    u32 local_height;
} tt_bitmap_view;

tt_bitmap tt_render_font_atlas(
    mem_arena* arena,
    const tt_font_info* font_info,
    u32* codepoints,
    // out_infos must be of size num_codepoints, 
    // and will be set by the function
    tt_render_glyph_info* out_infos,
    u32 num_codepoints,
    tt_render_mode render_mode,
    f32 glyph_scale,
    u32 pixel_dist_falloff,
    u32 bitmap_width
);

void tt_render_glyph(
    const tt_font_info* font_info,
    u32 glyph_index,
    tt_render_mode render_mode, 
    f32 glyph_scale,
    u32 pixel_dist_falloff,
    tt_bitmap_view* bitmap_view
);

