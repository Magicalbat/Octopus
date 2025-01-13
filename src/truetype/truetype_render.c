#include "truetype_render.h"

#include "base/base.h"

#include <math.h>

void _tt_render_glyph_sdf_impl(const tt_font_info* font_info, u32 glyph_index, f32 glyph_scale, u32 pixel_dist_falloff, tt_bitmap_view* bitmap_view, tt_segment* segments) {
    if (bitmap_view->offset_x >= bitmap_view->total_width || bitmap_view->offset_y >= bitmap_view->total_height) {
        return;
    }

    // Flip y to make +y down (for bitmap)
    mat2f transform = { .m = {
        glyph_scale, 0.0f,
        0.0f, -glyph_scale
    }};

    tt_bounding_box box = tt_get_glyph_box(font_info, glyph_index);

    vec2f offset = {
        -box.x_min * glyph_scale + pixel_dist_falloff, box.y_max * glyph_scale + pixel_dist_falloff
    };

    u32 num_segments = tt_get_glyph_outline(
        font_info, glyph_index, segments, 0, transform, offset
    );

    if (num_segments == 0) {
        return;
    }

    f32 dist_scale = 127.5f / (f32)pixel_dist_falloff;

    u32 clamped_width = MIN(bitmap_view->local_width, bitmap_view->total_width - bitmap_view->offset_x);
    u32 clamped_height = MIN(bitmap_view->local_height, bitmap_view->total_height - bitmap_view->offset_y);

    for (u32 local_y = 0; local_y < clamped_height; local_y++) {
        for (u32 local_x = 0; local_x < clamped_width; local_x++) {
            vec2f glyph_space_pos = {
                (f32)local_x + 0.5f,
                (f32)local_y + 0.5f
            };

            curve_dist_info min_dist = { .dist = INFINITY };

            for (u32 i = 0; i < num_segments; i++) {
                curve_dist_info cur_dist = tt_segment_dist(&segments[i], glyph_space_pos);

                if (cur_dist.dist < min_dist.dist) {
                    min_dist = cur_dist;
                } else if (ABS(cur_dist.dist - min_dist.dist) < 1e-6f) {
                    if (cur_dist.alignment < min_dist.alignment) {
                        min_dist = cur_dist;
                    }
                }
            }


            u32 img_x = local_x + bitmap_view->offset_x;
            u32 img_y = local_y + bitmap_view->offset_y;
            
            f32 val = min_dist.dist * (f32)min_dist.dist_sign * dist_scale;
            val += 127.5f;
            val = CLAMP(val, 0, 255);

            bitmap_view->data[img_x + img_y * bitmap_view->total_width] = (u8)val;
        }
    }
}

void tt_render_glyph_sdf(const tt_font_info* font_info, u32 glyph_index, f32 glyph_scale, u32 pixel_dist_falloff, tt_bitmap_view* bitmap_view) {
    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    tt_segment* segments = ARENA_PUSH_ARRAY(scratch.arena, tt_segment, font_info->max_glyph_points);
    _tt_render_glyph_sdf_impl(font_info, glyph_index, glyph_scale, pixel_dist_falloff, bitmap_view, segments);

    arena_scratch_release(scratch);
}

