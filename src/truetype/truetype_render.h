#ifndef TRUETYPE_RENDER_H
#define TRUETYPE_RENDER_H

#include "base/base_defs.h"

#include "truetype_parse.h"

typedef enum {
    TT_RENDER_SDF,
    TT_RENDER_MSDF,
    TT_RENDER_TSDF,
} tt_render_mode;

typedef struct {
    u8* data;
    u32 total_width;
    u32 total_height;

    u32 offset_x;
    u32 offset_y;
    u32 local_width;
    u32 local_height;
} tt_bitmap_view;

void tt_render_glyph_sdf(const tt_font_info* font_info, u32 glyph_index, f32 glyph_scale, u32 pixel_dist_falloff, tt_bitmap_view* bitmap_view);

#endif // TRUETYPE_RENDER_H

