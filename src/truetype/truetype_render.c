typedef void (_tt_render_func)(const tt_font_info* font_info, u32 glyph_index, f32 glyph_scale, u32 pixel_dist_falloff, tt_bitmap_view* bitmap_view, tt_segment* segments);

void _tt_render_sdf(const tt_font_info* font_info, u32 glyph_index, f32 glyph_scale, u32 pixel_dist_falloff, tt_bitmap_view* bitmap_view, tt_segment* segments);
void _tt_render_msdf(const tt_font_info* font_info, u32 glyph_index, f32 glyph_scale, u32 pixel_dist_falloff, tt_bitmap_view* bitmap_view, tt_segment* segments);
void _tt_render_tmsdf(const tt_font_info* font_info, u32 glyph_index, f32 glyph_scale, u32 pixel_dist_falloff, tt_bitmap_view* bitmap_view, tt_segment* segments);

static _tt_render_func* _render_funcs[TT_RENDER_COUNT] = {
    [TT_RENDER_SDF] = _tt_render_sdf,
    [TT_RENDER_MSDF] = _tt_render_msdf,
    [TT_RENDER_TMSDF] = _tt_render_tmsdf,
};

static u8 _pixel_sizes[TT_RENDER_COUNT] = {
    [TT_RENDER_SDF] = 1,
    [TT_RENDER_MSDF] = 3,
    [TT_RENDER_TMSDF] = 4
};

tt_bitmap tt_render_font_atlas(mem_arena* arena, const tt_font_info* font_info, u32* codepoints, rectf* out_rects, u32 num_codepoints, tt_render_mode render_mode, f32 glyph_scale, u32 pixel_dist_falloff, u32 bitmap_width) {
    if (
        font_info == NULL || !font_info->initialized ||
        codepoints == NULL || out_rects == NULL ||
        render_mode >= TT_RENDER_COUNT
    ) {
        return (tt_bitmap){ 0 };
    }

    mem_arena_temp scratch = arena_scratch_get(&arena, 1);

    u32 num_glyphs = num_codepoints;

    u32* glyph_indices = ARENA_PUSH_ARRAY(scratch.arena, u32, num_glyphs);
    for (u32 i = 0; i < num_glyphs; i++) {
        glyph_indices[i] = tt_get_glyph_index(font_info, codepoints[i]);
    }

    for (u32 i = 0; i < num_glyphs; i++) {
        tt_bounding_box box = tt_get_glyph_box(font_info, glyph_indices[i]);

        out_rects[i].w = ceilf((f32)(box.x_max - box.x_min) * glyph_scale + (f32)pixel_dist_falloff * 2);
        out_rects[i].h = ceilf((f32)(box.y_max - box.y_min) * glyph_scale + (f32)pixel_dist_falloff * 2);
    }

    u32 bitmap_height = (u32)ceilf(rectf_pack(out_rects, num_glyphs, (f32)bitmap_width, 1));

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
        bitmap_view.offset_x = (u32)out_rects[i].x;
        bitmap_view.offset_y = (u32)out_rects[i].y;
        bitmap_view.local_width = (u32)out_rects[i].w;
        bitmap_view.local_height = (u32)out_rects[i].h;

        render_func(font_info, glyph_indices[i], glyph_scale, pixel_dist_falloff, &bitmap_view, segments);
    }

    arena_scratch_release(scratch);

    return bitmap;
}

void tt_render_glyph(const tt_font_info* font_info, u32 glyph_index, tt_render_mode render_mode, f32 glyph_scale, u32 pixel_dist_falloff, tt_bitmap_view* bitmap_view) {
    if (font_info == NULL || !font_info->initialized || bitmap_view == NULL || render_mode >= TT_RENDER_COUNT) {
        return;
    }

    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    tt_segment* segments = ARENA_PUSH_ARRAY(scratch.arena, tt_segment, font_info->max_glyph_points);
    _render_funcs[render_mode](font_info, glyph_index, glyph_scale, pixel_dist_falloff, bitmap_view, segments);

    arena_scratch_release(scratch);
}


void _tt_render_sdf(const tt_font_info* font_info, u32 glyph_index, f32 glyph_scale, u32 pixel_dist_falloff, tt_bitmap_view* bitmap_view, tt_segment* segments) {
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
        -box.x_min * glyph_scale + (f32)pixel_dist_falloff, box.y_max * glyph_scale + (f32)pixel_dist_falloff
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

f32 _tt_calc_psuedo_dist(const tt_segment* segment, curve_dist_info dist, vec2f target) {
    if (dist.t > 0.0f || dist.t < 1.0f) {
        return dist.dist * (f32)dist.dist_sign;
    }

    vec2f dir = vec2f_norm(tt_segment_deriv(segment, dist.t));
    vec2f target_vec = vec2f_sub(target, tt_segment_point(segment, dist.t));

    f32 pseudo_dist = vec2f_cross(target_vec, dir);

    if (ABS(pseudo_dist) < dist.dist) {
        return pseudo_dist;
    }

    return dist.dist * (f32)dist.dist_sign;
}

#define _WHITE (TT_SEGMENT_FLAG_RED | TT_SEGMENT_FLAG_GREEN | TT_SEGMENT_FLAG_BLUE)
#define _MAGENTA (TT_SEGMENT_FLAG_RED | TT_SEGMENT_FLAG_BLUE)
#define _YELLOW (TT_SEGMENT_FLAG_RED | TT_SEGMENT_FLAG_GREEN)
#define _CYAN (TT_SEGMENT_FLAG_GREEN | TT_SEGMENT_FLAG_BLUE)

void _tt_render_msdf_impl(const tt_font_info* font_info, u32 glyph_index, f32 glyph_scale, u32 pixel_dist_falloff, tt_bitmap_view* bitmap_view, tt_segment* segments, b32 alpha)  {
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
        -box.x_min * glyph_scale + (f32)pixel_dist_falloff, box.y_max * glyph_scale + (f32)pixel_dist_falloff
    };

    u32 num_segments = tt_get_glyph_outline(
        font_info, glyph_index, segments, 0, transform, offset
    );

    if (num_segments == 0) {
        return;
    }
    
    u32 contour_len = num_segments;
    for (u32 contour_start = 0; contour_start < num_segments; contour_start += contour_len) {
        u32 j = contour_start + 1;
        while (j < num_segments && !(segments[j].flags & TT_SEGMENT_FLAG_CONTOUR_START)) {
            j++;
        }
        contour_len = j - contour_start;

        // The first segment might not be the start of its edge,
        // but the segments before are at the end of the contour in memory.
        // This finds out how many are in the same edge
        u32 num_end_in_first_edge = 0;
        if (!_tt_is_corner(&segments[contour_start + contour_len - 1], &segments[contour_start])) {
            num_end_in_first_edge++;

            for (u32 i = contour_start + contour_len - 1; i >= contour_start + 1; i--) {
                if (_tt_is_corner(&segments[i-1], &segments[i])) {
                    break;
                } else {
                    num_end_in_first_edge++;
                }
            }
        }

        u32 color = 0;

        // Teardrop case is handled later
        if (num_end_in_first_edge == contour_len) {
            color = _WHITE;

            for (u32 i = 0; i < contour_len; i++) {
                segments[i + contour_start].flags |= color;
            }
        } else { // Multiple edges 
            color = _MAGENTA;

            u32 num_magenta = num_end_in_first_edge;

            for (u32 i = 0; i < num_end_in_first_edge; i++) {
                segments[contour_start + contour_len - i - 1].flags |= color;
            }

            for (u32 i = 0; i < contour_len - num_end_in_first_edge; i++) {
                segments[contour_start + i].flags |= color;

                num_magenta += color == _MAGENTA;

                u32 next_i = i + 1;
                if (next_i >= contour_len) {
                    next_i = 0;
                }

                if (_tt_is_corner(&segments[contour_start + i], &segments[contour_start + next_i])) {
                    if (color == _YELLOW) {
                        color = _CYAN;
                    } else {
                        color = _YELLOW;
                    }
                }
            }

            // If the entire contour is magenta, then this is the teardrop case,
            // Have to find the corner and fix the colors
            if (num_magenta == contour_len) {
                for (u32 i = 0; i < contour_len - num_end_in_first_edge; i++) {
                    segments[contour_start + i].flags &= ~((u32)_MAGENTA);
                    segments[contour_start + i].flags |= _YELLOW;
                }
            }
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

            curve_dist_info min_dists[3] = { 0 };
            min_dists[0].dist = INFINITY;
            min_dists[1].dist = INFINITY;
            min_dists[2].dist = INFINITY;

            i64 min_dist_segments[3] = { -1, -1, -1 };

            for (u32 i = 0; i < num_segments; i++) {
                curve_dist_info cur_dist = tt_segment_dist(&segments[i], glyph_space_pos);

                if (segments[i].flags & TT_SEGMENT_FLAG_RED && curve_dist_less(cur_dist, min_dists[0])) {
                    min_dists[0] = cur_dist;
                    min_dist_segments[0] = i;
                }
                if (segments[i].flags & TT_SEGMENT_FLAG_GREEN && curve_dist_less(cur_dist, min_dists[1])) {
                    min_dists[1] = cur_dist;
                    min_dist_segments[1] = i;
                }
                if (segments[i].flags & TT_SEGMENT_FLAG_BLUE && curve_dist_less(cur_dist, min_dists[2])) {
                    min_dists[2] = cur_dist;
                    min_dist_segments[2] = i;
                }
            }

            f32 final_dists[4] = { INFINITY, INFINITY, INFINITY, INFINITY };
            for (u32 i = 0; i < 3; i++) {
                if (min_dist_segments[i] == -1) {
                    continue;
                }

                final_dists[i] = _tt_calc_psuedo_dist(&segments[min_dist_segments[i]], min_dists[i], glyph_space_pos);
            }

            u32 img_x = local_x + bitmap_view->offset_x;
            u32 img_y = local_y + bitmap_view->offset_y;

            #if 1

            u32 pixel_size = 3;

            if (alpha) {
                pixel_size = 4;

                curve_dist_info min_dist = min_dists[0];
                for (u32 i = 1; i < 3; i++) {
                    if (curve_dist_less(min_dists[i], min_dist)) {
                        min_dist = min_dists[i];
                    }
                }

                final_dists[3] = min_dist.dist * (f32)min_dist.dist_sign;
            }

            for (u32 i = 0; i < pixel_size; i++) {
                f32 val = final_dists[i] * dist_scale;
                val += 127.5f;
                val = CLAMP(val, 0, 255);

                bitmap_view->data[(img_x + img_y * bitmap_view->total_width) * pixel_size + i] = (u8)val;
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
}

void _tt_render_msdf(const tt_font_info* font_info, u32 glyph_index, f32 glyph_scale, u32 pixel_dist_falloff, tt_bitmap_view* bitmap_view, tt_segment* segments) {
    _tt_render_msdf_impl(font_info, glyph_index, glyph_scale, pixel_dist_falloff, bitmap_view, segments, false);
}
void _tt_render_tmsdf(const tt_font_info* font_info, u32 glyph_index, f32 glyph_scale, u32 pixel_dist_falloff, tt_bitmap_view* bitmap_view, tt_segment* segments) {
    _tt_render_msdf_impl(font_info, glyph_index, glyph_scale, pixel_dist_falloff, bitmap_view, segments, true);
}


