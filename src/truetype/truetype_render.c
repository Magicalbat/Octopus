typedef void (_tt_render_func)(const tt_font_info* font_info, u32 glyph_index, f32 glyph_scale, u32 pixel_dist_falloff, tt_bitmap_view* bitmap_view, tt_segment* segments);

void _tt_render_sdf(const tt_font_info* font_info, u32 glyph_index, f32 glyph_scale, u32 pixel_dist_falloff, tt_bitmap_view* bitmap_view, tt_segment* segments);
void _tt_render_msdf(const tt_font_info* font_info, u32 glyph_index, f32 glyph_scale, u32 pixel_dist_falloff, tt_bitmap_view* bitmap_view, tt_segment* segments);
void _tt_render_tmsdf(const tt_font_info* font_info, u32 glyph_index, f32 glyph_scale, u32 pixel_dist_falloff, tt_bitmap_view* bitmap_view, tt_segment* segments);

static _tt_render_func* _render_funcs[TT_RENDER_COUNT] = {
    [TT_RENDER_SDF] = _tt_render_sdf,
    //[TT_RENDER_MSDF] = _tt_render_msdf,
    //[TT_RENDER_TMSDF] = _tt_render_tmsdf,
};

static u8 _pixel_sizes[TT_RENDER_COUNT] = {
    [TT_RENDER_SDF] = 1,
    [TT_RENDER_MSDF] = 3,
    [TT_RENDER_TMSDF] = 4
};

tt_bitmap tt_render_font_atlas(mem_arena* arena, const tt_font_info* font_info, u32* codepoints, tt_render_glyph_info* out_infos, u32 num_codepoints, tt_render_mode render_mode, f32 glyph_scale, u32 pixel_dist_falloff, u32 bitmap_width) {
    if (
        font_info == NULL || !font_info->initialized ||
        codepoints == NULL || out_infos == NULL ||
        render_mode >= TT_RENDER_COUNT
    ) {
        return (tt_bitmap){ 0 };
    }

    // TODO: implement msdf and remove
    if (render_mode != TT_RENDER_SDF) {
        error_emit("Unsupported truetype render mode. Please use TT_RENDER_SDF");
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

        glyph_rects[i].w = ceilf((f32)(box.x_max - box.x_min) * glyph_scale + (f32)pixel_dist_falloff * 2);
        glyph_rects[i].h = ceilf((f32)(box.y_max - box.y_min) * glyph_scale + (f32)pixel_dist_falloff * 2);

        out_infos[i].offset = (vec2f){
            box.x_min * glyph_scale,
            -box.y_min * glyph_scale
        };
    }

    u32 bitmap_height = (u32)ceilf(rectf_pack(glyph_rects, num_glyphs, (f32)bitmap_width, 1));

    vec2f uv_scale = {
        1.0f / (f32)bitmap_width,
        1.0f / (f32)bitmap_height
    };

    for (u32 i = 0; i < num_glyphs; i++) {
        out_infos[i].bitmap_rect = glyph_rects[i];
        out_infos[i].uv_rect = (rectf) {
            glyph_rects[i].x * uv_scale.x,
            glyph_rects[i].y * uv_scale.y,
            glyph_rects[i].w * uv_scale.x,
            glyph_rects[i].h * uv_scale.y,
        };

        tt_h_metrics h_metrics = tt_get_glyph_h_metrics(font_info, glyph_indices[i]);
        out_infos[i].x_advance = h_metrics.advance_width * glyph_scale;
        out_infos[i].offset.x = h_metrics.left_side_bearing * glyph_scale;
    }

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
        bitmap_view.offset_x = (u32)glyph_rects[i].x;
        bitmap_view.offset_y = (u32)glyph_rects[i].y;
        bitmap_view.local_width = (u32)glyph_rects[i].w;
        bitmap_view.local_height = (u32)glyph_rects[i].h;

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

