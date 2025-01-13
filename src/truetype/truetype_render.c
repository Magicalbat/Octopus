#include "truetype_render.h"

#include "base/base.h"

#include <math.h>

typedef void (_tt_render_func)(const tt_font_info* font_info, u32 glyph_index, f32 glyph_scale, u32 pixel_dist_falloff, tt_bitmap_view* bitmap_view, tt_segment* segments);

void _tt_render_glyph_sdf_impl(const tt_font_info* font_info, u32 glyph_index, f32 glyph_scale, u32 pixel_dist_falloff, tt_bitmap_view* bitmap_view, tt_segment* segments);

static _tt_render_func* _render_funcs[TT_RENDER_COUNT] = {
    [TT_RENDER_SDF] = _tt_render_glyph_sdf_impl,
    // TODO: make msdf and tmsdf functions
};

static u8 _pixel_sizes[TT_RENDER_COUNT] = {
    [TT_RENDER_SDF] = 1,
    [TT_RENDER_MSDF] = 3,
    [TT_RENDER_TMSDF] = 4
};

tt_bitmap tt_render_font_atlas(mem_arena* arena, const tt_font_info* font_info, u32* codepoints, u32 num_codepoints, tt_render_mode render_mode, f32 glyph_scale, u32 pixel_dist_falloff, u32 bitmap_width) {
    if (font_info == NULL || !font_info->initialized || codepoints == NULL || render_mode >= TT_RENDER_COUNT) {
        return (tt_bitmap){ 0 };
    }

    mem_arena_temp scratch = arena_scratch_get(&arena, 1);

    u32 num_glyphs = num_codepoints;

    u32* glyph_indices = ARENA_PUSH_ARRAY(scratch.arena, u32, num_glyphs);
    for (u32 i = 0; i < num_glyphs; i++) {
        glyph_indices[i] = tt_get_glyph_index(font_info, codepoints[i]);
    }

    rectf* glyph_rects = ARENA_PUSH_ARRAY(scratch.arena, rectf, num_glyphs);
    for (u32 i = 0; i < num_glyphs; i++) {
        tt_bounding_box box = tt_get_glyph_box(font_info, glyph_indices[i]);

        glyph_rects[i].w = ceilf((f32)(box.x_max - box.x_min) * glyph_scale + pixel_dist_falloff * 2);
        glyph_rects[i].h = ceilf((f32)(box.y_max - box.y_min) * glyph_scale + pixel_dist_falloff * 2);
    }

    u32 bitmap_height = ceilf(rectf_pack(glyph_rects, num_glyphs, bitmap_width, 1));

    tt_bitmap bitmap = {
        .data = ARENA_PUSH_ARRAY(arena, u8, _pixel_sizes[render_mode] * bitmap_width * bitmap_height),
        .width = bitmap_width,
        .height = bitmap_height
    };

    tt_bitmap_view bitmap_view = {
        .data = bitmap.data,
        .total_width = bitmap_width,
        .total_height = bitmap_height
    };
    _tt_render_func* render_func = _render_funcs[render_mode];

    tt_segment* segments = ARENA_PUSH_ARRAY(scratch.arena, tt_segment, font_info->max_glyph_points);

    for (u32 i = 0; i < num_glyphs; i++) {
        bitmap_view.offset_x = glyph_rects[i].x;
        bitmap_view.offset_y = glyph_rects[i].y;
        bitmap_view.local_width = glyph_rects[i].w;
        bitmap_view.local_height = glyph_rects[i].h;

        render_func(font_info, glyph_indices[i], glyph_scale, pixel_dist_falloff, &bitmap_view, segments);
    }

    arena_scratch_release(scratch);

    return bitmap;
}

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
    if (font_info == NULL || !font_info->initialized || bitmap_view == NULL) {
        return;
    }

    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    tt_segment* segments = ARENA_PUSH_ARRAY(scratch.arena, tt_segment, font_info->max_glyph_points);
    _tt_render_glyph_sdf_impl(font_info, glyph_index, glyph_scale, pixel_dist_falloff, bitmap_view, segments);

    arena_scratch_release(scratch);
}

