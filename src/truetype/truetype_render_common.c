
#define _TT_CORNER_DOT_THRESH 0.99f

#define _TT_WHITE (TT_POINT_FLAG_RED | TT_POINT_FLAG_GREEN | TT_POINT_FLAG_BLUE)
#define _TT_CYAN (TT_POINT_FLAG_GREEN | TT_POINT_FLAG_BLUE)
#define _TT_MAGENTA (TT_POINT_FLAG_RED | TT_POINT_FLAG_BLUE)
#define _TT_YELLOW (TT_POINT_FLAG_RED | TT_POINT_FLAG_GREEN)

void tt_glyph_color_edges(tt_glyph_data* glyph) {
    u32 index = 0;
    
    for (u32 contour = 0; contour < glyph->num_contours; contour++) {
        u32 num_corners = 0;
        u32 contour_start = index;
        u32 edge_start = index;

        u32 color = _TT_MAGENTA;

        v2_f32 first_start_dir = { 1, 0 };
        v2_f32 prev_end_dir = { 1, 0 };

        while (
            index < glyph->num_points &&
            (glyph->flags[index] & TT_POINT_FLAG_CONTOUR_END) != TT_POINT_FLAG_CONTOUR_END
        ) {
            b32 first_segment = index == contour_start;

            u32 num_points = 0;
            v2_f32 points[3] = { 0 };

            if (glyph->flags[index] & TT_POINT_FLAG_LINE) {
                v2_i16 p0 = glyph->points[index++];
                v2_i16 p1 = glyph->points[index];

                num_points = 2;
                points[0] = (v2_f32){ p0.x, -p0.y };
                points[1] = (v2_f32){ p1.x, -p1.y };
            } else {
                v2_i16 p0 = glyph->points[index++];
                v2_i16 p1 = glyph->points[index++];
                v2_i16 p2 = glyph->points[index];

                num_points = 3;
                points[0] = (v2_f32){ p0.x, -p0.y };
                points[1] = (v2_f32){ p1.x, -p1.y };
                points[2] = (v2_f32){ p2.x, -p2.y };
            }

            v2_f32 start_dir = v2_f32_norm(v2_f32_sub(points[1], points[0]));

            if (first_segment) {
                first_start_dir = start_dir;
            } else if (v2_f32_dot(prev_end_dir, start_dir) < _TT_CORNER_DOT_THRESH) {
                if (color == _TT_YELLOW) {
                    color = _TT_CYAN;
                } else {
                    color = _TT_YELLOW;
                }

                num_corners++;
                edge_start = index + 1 - num_points;
            }

            glyph->flags[index + 1 - num_points] |= color;

            prev_end_dir = v2_f32_norm(v2_f32_sub(points[num_points-1], points[num_points-2]));
        }

        if (num_corners == 0) {
            for (u32 i = contour_start; i <= index; i++) {
                glyph->flags[i] |= _TT_WHITE;
            }
        } else if (
            // To avoid the teadrop case
            num_corners > 1 &&
            v2_f32_dot(prev_end_dir, first_start_dir) >= _TT_CORNER_DOT_THRESH
        ) {
            for (u32 i = edge_start; i <= index; i++) {
                glyph->flags[i] &= ~_TT_WHITE;
                glyph->flags[i] |= _TT_MAGENTA;
            }
        }

        index++;
    }
}

