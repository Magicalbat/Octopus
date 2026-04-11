
void tt_raster_glyph_sdf(
    bitmap_r8* bmp, v2_i32 offset, tt_glyph_data* glyph,
    f32 scale, u32 padding, f32 dist_px_range
) {
    if (
        offset.x < 0 || offset.x >= (i32)bmp->width ||
        offset.y < 0 || offset.y >= (i32)bmp->height
    ) {
        return;
    }

    f32 x_min_scaled = (f32)glyph->x_min *  scale;
    f32 x_max_scaled = (f32)glyph->x_max *  scale;

    // TTF uses +y up, bitmaps use +y down
    // hence the switch
    f32 y_min_scaled = (f32)glyph->y_max * -scale;
    f32 y_max_scaled = (f32)glyph->y_min * -scale;

    u32 width = (u32)ceilf(x_max_scaled - x_min_scaled) + padding * 2;
    u32 height = (u32)ceilf(y_max_scaled - y_min_scaled) + padding * 2;

    width = MIN(width, bmp->width - (u32)offset.x);
    height = MIN(height, bmp->height - (u32)offset.y);

    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    v2_f32* points = PUSH_ARRAY_NZ(scratch.arena, v2_f32, glyph->num_points);
    for (u32 i = 0; i < glyph->num_points; i++) {
        points[i].x = (f32)glyph->points[i].x *  scale - x_min_scaled + (f32)padding;
        points[i].y = (f32)glyph->points[i].y * -scale - y_min_scaled + (f32)padding;
    }

    for (u32 y_i = 0; y_i < height; y_i++) {
        for (u32 x_i = 0; x_i < height; x_i++) {
            v2_f32 p = { (f32)x_i + 0.5f, (f32)y_i + 0.5f };

            f32 min_dist = INFINITY;

            u32 index = 0;
            for (u32 seg = 0; seg < glyph->num_segments; seg++) {
                f32 dist = INFINITY;

                if (glyph->flags[index] & TT_POINT_FLAG_LINE) {
                    v2_f32 p0 = points[index++];
                    v2_f32 p1 = points[index];

                    v2_f32 line_vec = v2_f32_sub(p1, p0);
                    v2_f32 point_vec = v2_f32_sub(p, p0);

                    f32 t = v2_f32_dot(point_vec, line_vec) / v2_f32_dot(line_vec, line_vec);
                    t = CLAMP(t, 0.0f, 1.0f);

                    v2_f32 line_point = v2_f32_add(p0, v2_f32_scale(line_vec, t));
                    dist = v2_f32_dist(p, line_point);
                } else {
                    v2_f32 p0 = points[index++];
                    v2_f32 p1 = points[index++];
                    v2_f32 p2 = points[index];

                    v2_f32 c0 = v2_f32_sub(p, p0);
                    v2_f32 c1 = v2_f32_sub(p1, p0);
                    v2_f32 c2 = v2_f32_add(p2, v2_f32_add(v2_f32_scale(p1, -2.0f), p0));

                    f32 ts[3] = { 0 };
                    u32 num_t = solve_cubic(
                        ts,
                        v2_f32_dot(c2, c2),
                        3.0f * v2_f32_dot(c1, c2),
                        2.0f * v2_f32_dot(c1, c1) - v2_f32_dot(c2, c0),
                        -v2_f32_dot(c1, c0)
                    );

                    for (u32 i = 0; i < num_t; i++) {
                        f32 t = CLAMP(ts[i], 0.0f, 1.0f);

                        v2_f32 bez_point = v2_f32_add(
                            v2_f32_add(
                                v2_f32_scale(c2, t * t),
                                v2_f32_scale(c1, t)
                            ), p0
                        );

                        f32 cur_dist = v2_f32_dist(p, bez_point);
                        dist = MIN(dist, cur_dist);
                    }
                }

                min_dist = MIN(min_dist, dist);

                if (glyph->flags[index] & TT_POINT_FLAG_CONTOUR_END) {
                    index++;
                }
            }

            f32 scaled_dist = CLAMP(min_dist, -dist_px_range, dist_px_range);
            scaled_dist *= 255.0f / (dist_px_range);

            u32 bmp_index = (x_i + (u32)offset.x) + (y_i + (u32)offset.y) * bmp->width;
            bmp->data[bmp_index] = (u8)scaled_dist;
        }
    }

    arena_scratch_release(scratch);
}

