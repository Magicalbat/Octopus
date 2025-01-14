#include "truetype_render.h"

#include "base/base.h"

#include <math.h>

typedef void (_tt_render_func)(const tt_font_info* font_info, u32 glyph_index, f32 glyph_scale, u32 pixel_dist_falloff, tt_bitmap_view* bitmap_view, tt_segment* segments);

void _tt_render_glyph_sdf_impl(const tt_font_info* font_info, u32 glyph_index, f32 glyph_scale, u32 pixel_dist_falloff, tt_bitmap_view* bitmap_view, tt_segment* segments);
void _tt_render_glyph_msdf_impl(const tt_font_info* font_info, u32 glyph_index, f32 glyph_scale, u32 pixel_dist_falloff, tt_bitmap_view* bitmap_view, tt_segment* segments);

static _tt_render_func* _render_funcs[TT_RENDER_COUNT] = {
    [TT_RENDER_SDF] = _tt_render_glyph_sdf_impl,
    [TT_RENDER_MSDF] = _tt_render_glyph_msdf_impl,
    // TODO: TMSDF
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

                if (curve_dist_less(cur_dist, min_dist)) {
                    min_dist = cur_dist;
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

b32 _tt_is_corner(const tt_segment* a, const tt_segment* b) {
    vec2f a_dir = vec2f_norm(tt_segment_deriv(a, 1.0f));
    vec2f b_dir = vec2f_norm(tt_segment_deriv(b, 0.0f));
    f32 dot = vec2f_dot(a_dir, b_dir);

    return dot < TT_CORNER_DOT_THRESHOLD;
}

// 1 bit for each color
typedef enum {
    _TT_EDGE_FLAG_CONTOUR_START = (1 << 0),

    _TT_EDGE_FLAG_RED = (1 << 1),
    _TT_EDGE_FLAG_GREEN = (1 << 2),
    _TT_EDGE_FLAG_BLUE = (1 << 3),
} _tt_edge_col;

typedef struct {
    tt_segment* segments;
    u32 num_segments;

    u32 flags;
} _tt_edge;

curve_dist_info _tt_edge_dist(const _tt_edge* edge, vec2f target) {
    curve_dist_info min_dist = { .dist=INFINITY };

    for (u32 i = 0; i < edge->num_segments; i++) {
        curve_dist_info dist = tt_segment_dist(&edge->segments[i], target);

        if (curve_dist_less(dist, min_dist)) {
            min_dist = dist;
        }
    }

    return min_dist;
}

f32 _tt_edge_psuedo_dist(const _tt_edge* edge, vec2f target) {
    curve_dist_info min_dist = _tt_edge_dist(edge, target);

    vec2f start_point = tt_segment_point(&edge->segments[0], 0.0f);
    vec2f start_deriv = tt_segment_deriv(&edge->segments[0], 0.0f);
    vec2f end_point = tt_segment_point(&edge->segments[edge->num_segments - 1], 1.0f);
    vec2f end_deriv = tt_segment_deriv(&edge->segments[edge->num_segments - 1], 1.0f);

    if (!vec2f_eq(start_point, end_point)) {
        tt_segment end_segs[] = {
            (tt_segment){
                .type = TT_SEGMENT_LINE,
                .line = (line2f) {
                    vec2f_sub(start_point, vec2f_scale(start_deriv, 1e6f)),
                    start_point
                }
            },
            (tt_segment){
                .type = TT_SEGMENT_LINE,
                .line = (line2f) {
                    end_point,
                    vec2f_add(end_point, vec2f_scale(end_deriv, 1e6f))
                }
            },
        };

        _tt_edge end_edge = {
            .segments = end_segs,
            .num_segments = 2
        };

        curve_dist_info end_dist = _tt_edge_dist(&end_edge, target);
        
        if (curve_dist_less(end_dist, min_dist)) {
            min_dist = end_dist;
        }
    }

    return min_dist.dist * min_dist.dist_sign;
}

// TODO: remove
#include <stdio.h>
void _tt_render_glyph_msdf_impl(const tt_font_info* font_info, u32 glyph_index, f32 glyph_scale, u32 pixel_dist_falloff, tt_bitmap_view* bitmap_view, tt_segment* segments)  {
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

    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    _tt_edge* edges = ARENA_PUSH_ARRAY(scratch.arena, _tt_edge, num_segments);
    u32 num_edges = 0;

    u32 contour_len = num_segments-1;
    for (u32 contour_start = 0; contour_start < num_segments; contour_start += contour_len) {
        // Finding contour length
        {
            u32 j = 0;
            for (j = contour_start + 1; j < num_segments; j++) {
                if (segments[j].contour_start) {
                    break;
                }
            }

            contour_len = j - contour_start;
        }

        // Rotating until the first segment of the contour is the start of an edge
        // There is a possibility that there are no corners on the contour, 
        // so I am wrapping it in a for loop
        for (u32 j = 0; j < contour_len; j++) {
            if (_tt_is_corner(&segments[contour_start + contour_len - 1], &segments[contour_start])) {
                break;
            } else {
                tt_segment last_seg = segments[contour_start + contour_len - 1];

                for (i64 k = contour_len - 1; k >= 1; k--) {
                    segments[contour_start + k] = segments[contour_start + k - 1];
                }
                segments[contour_start] = last_seg;
            }
        }

        edges[num_edges++] = (_tt_edge){
            .segments = &segments[contour_start],
            .num_segments = 1,
            .flags = _TT_EDGE_FLAG_CONTOUR_START
        };

        for (u32 j = contour_start + 1; j < contour_start + contour_len; j++) {
            if (_tt_is_corner(&segments[j-1], &segments[j])) {
                edges[num_edges++] = (_tt_edge){
                    .segments = &segments[j],
                    .num_segments = 1,
                };
            } else {
                edges[num_edges-1].num_segments++;
            }
        }
    }

    u32 cur_color = 0;
    for (u32 i = 0; i < num_edges; i++) {
        if (edges[i].flags & _TT_EDGE_FLAG_CONTOUR_START) {
            if (i < num_edges-1 && (edges[i+1].flags & _TT_EDGE_FLAG_CONTOUR_START)) {
                cur_color = _TT_EDGE_FLAG_RED | _TT_EDGE_FLAG_GREEN | _TT_EDGE_FLAG_BLUE;
            } else {
                cur_color = _TT_EDGE_FLAG_RED | _TT_EDGE_FLAG_BLUE;
            }
        }

        edges[i].flags |= cur_color;

        if (cur_color == (_TT_EDGE_FLAG_RED | _TT_EDGE_FLAG_GREEN)) {
            cur_color = _TT_EDGE_FLAG_GREEN | _TT_EDGE_FLAG_BLUE;
        } else {
            cur_color = _TT_EDGE_FLAG_RED | _TT_EDGE_FLAG_GREEN;
        }
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

            curve_dist_info min_dists[3] = {};
            min_dists[0].dist = INFINITY;
            min_dists[1].dist = INFINITY;
            min_dists[2].dist = INFINITY;

            i64 min_dist_edges[3] = { -1, -1, -1 };

            for (u32 i = 0; i < num_edges; i++) {
                curve_dist_info cur_dist = _tt_edge_dist(&edges[i], glyph_space_pos);

                if (edges[i].flags & _TT_EDGE_FLAG_RED && curve_dist_less(cur_dist, min_dists[0])) {
                    min_dists[0] = cur_dist;
                    min_dist_edges[0] = i;
                }
                if (edges[i].flags & _TT_EDGE_FLAG_GREEN && curve_dist_less(cur_dist, min_dists[1])) {
                    min_dists[1] = cur_dist;
                    min_dist_edges[1] = i;
                }
                if (edges[i].flags & _TT_EDGE_FLAG_BLUE && curve_dist_less(cur_dist, min_dists[2])) {
                    min_dists[2] = cur_dist;
                    min_dist_edges[2] = i;
                }
            }

            f32 final_dists[3] = { INFINITY, INFINITY, INFINITY };
            for (u32 i = 0; i < 3; i++) {
                if (min_dist_edges[i] == -1) {
                    continue;
                }

                final_dists[i] = _tt_edge_psuedo_dist(&edges[min_dist_edges[i]], glyph_space_pos);
            }

            u32 img_x = local_x + bitmap_view->offset_x;
            u32 img_y = local_y + bitmap_view->offset_y;

            #if 1

            for (u32 i = 0; i < 3; i++) {
                f32 val = final_dists[i] * dist_scale;
                val += 127.5f;
                val = CLAMP(val, 0, 255);

                bitmap_view->data[(img_x + img_y * bitmap_view->total_width) * 3 + i] = (u8)val;
            }

            #else

            // Edge coloring view, for debugging
            for (u32 i = 0; i < 3; i++) {
                f32 val = min_dists[i].dist * (f32)min_dists[i].dist_sign * dist_scale;
                val = (ABS(val) < 32) * 255;
                bitmap_view->data[(img_x + img_y * bitmap_view->total_width) * 3 + i] = val;
            }

             #endif

        }
    }

    arena_scratch_release(scratch);
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

void tt_render_glyph_msdf(const tt_font_info* font_info,u32 glyph_index,f32 glyph_scale,u32 pixel_dist_falloff,tt_bitmap_view* bitmap_view) {
    if (font_info == NULL || !font_info->initialized || bitmap_view == NULL) {
        return;
    }

    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    tt_segment* segments = ARENA_PUSH_ARRAY(scratch.arena, tt_segment, font_info->max_glyph_points);
    _tt_render_glyph_msdf_impl(font_info, glyph_index, glyph_scale, pixel_dist_falloff, bitmap_view, segments);

    arena_scratch_release(scratch);

}

