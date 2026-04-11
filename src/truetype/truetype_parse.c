
#define _TT_READ_BE16(m) (u16)( \
    ((u16)((u8*)(m))[0] <<  8) | \
    ((u16)((u8*)(m))[1]))

#define _TT_READ_BE32(m) (u32)( \
    ((u32)((u8*)(m))[0] << 24) | \
    ((u32)((u8*)(m))[1] << 16) | \
    ((u32)((u8*)(m))[2] <<  8) | \
    ((u32)((u8*)(m))[3]))

#define _TT_TAG(s) _TT_READ_BE32((s))

typedef struct {
    // From the start of the glyf table
    u32 offset;
    u32 length;
} _tt_glyf_entry;

u32 _tt_calc_checksum(string8 file, u32 offset, u32 len);
// Does not bounds check table records
b32 _tt_get_validate_table(string8 file, u32 table_tag, tt_font_table* table);
// Assumes loca is already validated
b32 _tt_validate_loca(string8 file, const tt_font_info* info);
// Assumes cmap is already validated
b32 _tt_find_cmap_subtable(string8 file, tt_font_info* info, tt_font_table cmap);
_tt_glyf_entry _tt_find_glyf_entry(string8 file, tt_font_info* info, u32 glyph_index);

void tt_font_init(string8 file, tt_font_info* info) {
    if (file.size <= 12) {
        error_emit("Cannit parse TTF (invalid file)");
        goto invalid; 
    }

    u32 scalar_type = _TT_READ_BE32(file.str);
    if (scalar_type != 0x00010000  && scalar_type != 0x74727565) {
        error_emit("Cannot parse TTF (invalid file type)");
        goto invalid;
    }

    u32 file_checksum = _tt_calc_checksum(file, 0, (u32)file.size);
    if (file_checksum != 0xB1B0AFBA) {
        error_emit("Invalid/corrupted TTF file (checksum failed)");
        goto invalid;
    }

    u16 num_tables = _TT_READ_BE16(file.str + 4);
    if (file.size <= 12 + num_tables * 16) {
        error_emit("Cannot parse TTF (invalid table records)");
        goto invalid;
    }

    tt_font_table cmap = { 0 };
    tt_font_table maxp = { 0 };

    if (
        !_tt_get_validate_table(file, _TT_TAG("head"), &info->head) ||
        !_tt_get_validate_table(file, _TT_TAG("glyf"), &info->glyf) ||
        !_tt_get_validate_table(file, _TT_TAG("hmtx"), &info->hmtx) ||
        !_tt_get_validate_table(file, _TT_TAG("loca"), &info->loca) ||
        !_tt_get_validate_table(file, _TT_TAG("cmap"), &cmap) ||
        !_tt_get_validate_table(file, _TT_TAG("maxp"), &maxp) ||
        info->head.length != 54 || maxp.length < 6
    ) {
        error_emit("Cannot parse TTF (invalid tables)");
        goto invalid;
    }

    info->loca_format = (i16)_TT_READ_BE16(file.str + info->head.offset + 50);
    if (info->loca_format != 0 && info->loca_format != 1) {
        error_emit("Cannot parse TTF (invalid loca format)");
        goto invalid;
    }

    if (!_tt_validate_loca(file, info)) {
        error_emit("Cannot parse TTF (invalid loca offsets)");
        goto invalid;
    }

    // maxp parsing
    {
        if (_TT_READ_BE32(file.str + maxp.offset) != 0x00010000 || maxp.length != 32) {
            error_emit("Cannot parse TTF (CFF glyphs not supported yet)");
            goto invalid;
        }

        info->num_glyphs = _TT_READ_BE16(file.str + maxp.offset + 4);
        u32 loca_size = info->loca.length / (info->loca_format == 0 ? 2 : 4);
        if (loca_size < info->num_glyphs + 1) {
            error_emit("Cannot parse TTF (invalid loca size)");
            goto invalid;
        }

        u16 max_points = _TT_READ_BE16(file.str + maxp.offset + 6);
        u16 max_composite_points = _TT_READ_BE16(file.str + maxp.offset + 10);
        u16 max_contours = _TT_READ_BE16(file.str + maxp.offset + 8);
        u16 max_composite_contours = _TT_READ_BE16(file.str + maxp.offset + 8);

        info->max_glyph_contours = (u32)MAX(max_contours, max_composite_contours);

        // The processing of glyphs involves generating implied control points
        // and duplicating the first point to close the contour
        // These calculations account for that
        info->max_glyph_points =
            (u32)MAX(max_points, max_composite_points) * 2 +
            info->max_glyph_contours;
    }

    if (!_tt_find_cmap_subtable(file, info, cmap)) {
        error_emit("Cannot parse TTF (unable to find supported cmap)");
        goto invalid;
    }

    info->initialized = true;

    return;

invalid:
    info->initialized = false;
}

f32 tt_scale_for_em(string8 file, tt_font_info* info, f32 pixels_per_em) {
    if (info == NULL || !info->initialized) { return 1.0f; }

    f32 units_per_em = (f32)_TT_READ_BE16(file.str + info->head.offset + 18);
    return pixels_per_em / units_per_em;
}

// Used for loading in glyphs before the number of total points is known
typedef struct {
    u32 num_segments;
    u32 num_points;

    u32 max_points;

    tt_point_flag* flags;
    v2_i16* points;
} _tt_temp_glyph;

// Recursively adds points to the `_tt_temp_glyph` structure
b32 _tt_glyph_add_points(
    string8 file, tt_font_info* info,
    _tt_temp_glyph* glyph, u32 glyph_index
) {
    _tt_glyf_entry entry = _tt_find_glyf_entry(file, info, glyph_index);
    if (entry.length < 10) { return false; }

    u8* glyf_data = file.str + info->glyf.offset + entry.offset;

    i16 num_contours = (i16)_TT_READ_BE16(glyf_data);

    if (num_contours > 0) {
        // Simple glyph description
        if (entry.length < 12 + (u32)num_contours * 2) {
            return false; 
        }

        u16 instruction_length = _TT_READ_BE16(glyf_data + 10 + num_contours * 2);
        if (entry.length < 12 + (u32)num_contours * 2 + instruction_length) {
            return false;
        }

        u32 num_raw_points = (u32)_TT_READ_BE16(glyf_data + 10 + (num_contours - 1) * 2) + 1;

        mem_arena_temp scratch = arena_scratch_get(NULL, 0);

        u8* flags_raw = PUSH_ARRAY(scratch.arena, u8, num_raw_points);
        v2_i16* points_raw = PUSH_ARRAY(scratch.arena, v2_i16, num_raw_points);

        u32 data_offset = 12 + (u32)num_contours * 2 + instruction_length;
        u32 num_flags = 0;

        while (num_flags < num_raw_points && data_offset < entry.length) {
            u8 flag = *(glyf_data + data_offset);
            data_offset++;

            flags_raw[num_flags++] = flag;

            // Checking REPEAT flag
            if ((flag & 0x08) && data_offset < entry.length) {
                u8 count = *(glyf_data + data_offset);
                data_offset++;

                while (count-- && num_flags < num_raw_points) {
                    flags_raw[num_flags++] = flag;
                }
            }
        }

        i16 x = 0;
        for (u32 i = 0; i < num_raw_points; i++) {
            if ((flags_raw[i] & 0x02) && data_offset < entry.length) {
                // 1 byte x
                u8 diff = *(glyf_data + data_offset);
                data_offset++;

                // Checking if diff is positive
                x += (flags_raw[i] & 0x10) ? (i16)diff : -(i16)diff;
            } else if ((flags_raw[i] & 0x10) != 0x10 && data_offset + 1 < entry.length) {
                // If x is not the same (0x10), then get the new diff
                i16 diff = (i16)_TT_READ_BE16(glyf_data + data_offset);
                data_offset += 2;

                x += diff;
            }

            points_raw[i].x = x;
        }

        i16 y = 0;
        for (u32 i = 0; i < num_raw_points; i++) {
            if ((flags_raw[i] & 0x04) && data_offset < entry.length) {
                // 1 byte y
                u8 diff = *(glyf_data + data_offset);
                data_offset++;

                // Checking if diff is positive
                y += (flags_raw[i] & 0x20) ? (i16)diff : -(i16)diff;
            } else if ((flags_raw[i] & 0x20) != 0x20 && data_offset + 1 < entry.length) {
                // If y is not the same (0x10), then get the new diff
                i16 diff = (i16)_TT_READ_BE16(glyf_data + data_offset);
                data_offset += 2;

                y += diff;
            }

            points_raw[i].y = y;
        }

        for (i32 c = 0; c < num_contours; c++) {
            u32 start_point = c <= 0 ? 0 : (_TT_READ_BE16(glyf_data + 10 + (c-1) * 2) + 1);
            u32 end_point = _TT_READ_BE16(glyf_data + 10 + c * 2);

            u32 num_points = end_point - start_point + 1;
            i32 point_offset = 0;
            b8 just_offset = false;
            for (i32 i = 0; i < (i32)num_points; i++) {
                u32 p0_i = (((u32)(i + point_offset + 0) % num_points) + start_point);
                u32 p1_i = (((u32)(i + point_offset + 1) % num_points) + start_point);
                u32 p2_i = (((u32)(i + point_offset + 2) % num_points) + start_point);

                v2_i16 p0 = points_raw[p0_i];
                v2_i16 p1 = points_raw[p1_i];
                v2_i16 p2 = points_raw[p2_i];

                tt_point_flag p0_flag = just_offset ? TT_POINT_CONTOUR_OFFSET : 0;
                tt_point_flag p1_flag = 0;
                tt_point_flag p2_flag = 0;

                just_offset = false;

                b8 bez = true, skip = false;

                u32 on_curve_bits = (u32)(
                    (flags_raw[p0_i] & 0x1) << 2 |
                    (flags_raw[p1_i] & 0x1) << 1 |
                    (flags_raw[p2_i] & 0x1) << 0
                );

                switch (on_curve_bits) {
                    case 0b110:
                    case 0b111: {
                        // Line Segment
                        bez = false;

                        p2 = p1;

                        p0_flag |= TT_POINT_FLAG_LINE;
                    } break;

                    case 0b101: {
                        // Beizer (all points explicit)

                        // Skip off curve point
                        i++;
                    } break;

                    case 0b100: {
                        // Bezier (end point implicit)
                        p2 = (v2_i16) {
                            (p1.x + p2.x) / 2,
                            (p1.y + p2.y) / 2
                        };

                        p2_flag |= TT_POINT_FLAG_GENERATED;
                    } break;

                    case 0b001: {
                        // Bezier (start point implicit)
                        p0 = (v2_i16) {
                            (p0.x + p1.x) / 2,
                            (p0.y + p1.y) / 2,
                        };

                        p0_flag |= TT_POINT_FLAG_GENERATED;

                        // Skip off curve point
                        i++;
                    } break;

                    case 0b000: {
                        // Bezier (start and end implicit)
                        p0 = (v2_i16) {
                            (p0.x + p1.x) / 2,
                            (p0.y + p1.y) / 2,
                        };
                        p2 = (v2_i16) {
                            (p1.x + p2.x) / 2,
                            (p1.y + p2.y) / 2
                        };

                        p0_flag |= TT_POINT_FLAG_GENERATED;
                        p2_flag |= TT_POINT_FLAG_GENERATED;
                    } break;

                    case 0b010:
                    case 0b011: {
                        // Undefined case, adjust offset until it makes sense
                        point_offset++;
                        i--;

                        just_offset = true;

                        skip = true;
                    } break;
                }

                if (skip) { continue; }

                glyph->num_segments++;

                glyph->num_points++;
                glyph->flags[glyph->num_points-1] = p0_flag;
                glyph->points[glyph->num_points-1] = p0;

                if (bez) {
                    glyph->num_points++;
                    glyph->flags[glyph->num_points-1] = p1_flag;
                    glyph->points[glyph->num_points-1] = p1;
                }

                if (i == (i32)num_points - 1) {
                    p2_flag |= TT_POINT_FLAG_CONTOUR_END;

                    glyph->num_points++;
                    glyph->flags[glyph->num_points-1] = p2_flag;
                    glyph->points[glyph->num_points-1] = p2;
                }
            }
        }

        arena_scratch_release(scratch);
    } else if (num_contours < 0) {
        // Compound glyph description

        u32 data_offset = 10;
        b8 more = true;
        while (more && data_offset < entry.length) {
            if (data_offset + 4 > entry.length) { return false; }

            u16 flags = _TT_READ_BE16(glyf_data + data_offset + 0);
            u16 child_glyph_index = _TT_READ_BE16(glyf_data + data_offset + 2);
            data_offset += 4;

            i16 raw_x_offset = 0, raw_y_offset = 0;
            i32 parent_point = -1, child_point = -1;

            b8 scale_offset = false;

            if (flags & 0x0002) {
                // ARGS_ARE_XY_VALUES
                if (flags & 0x0001 && data_offset + 4 <= entry.length) {
                    // 16 bit offsets
                    raw_x_offset = (i16)_TT_READ_BE16(glyf_data + data_offset + 0);
                    raw_y_offset = (i16)_TT_READ_BE16(glyf_data + data_offset + 2);

                    data_offset += 4;
                } else if (data_offset + 2 <= entry.length) {
                    // 8 bit offsets
                    raw_x_offset = (i8)(*(glyf_data + data_offset + 0));
                    raw_y_offset = (i8)(*(glyf_data + data_offset + 1));

                    data_offset += 2;
                }

                if (flags & 0x0800) {
                    // SCALED_COMPONENT_OFFSET
                    scale_offset = true;
                }

                // This is defined as the default behavior,
                // hence the if instead of else if
                if (flags & 0x1000) {
                    // UNSCALED_COMPONENT_OFFSET
                    scale_offset = false;
                }
            } else {
                // Not ARGS_ARE_XY_VALUES
                // (args are point numbers)
                if (flags & 0x0001 && data_offset + 4 <= entry.length) {
                    // 16 bit point indices
                    parent_point = _TT_READ_BE16(glyf_data + data_offset + 0);
                    child_point = _TT_READ_BE16(glyf_data + data_offset + 2);

                    data_offset += 4;
                } else if (data_offset + 2 <= entry.length) {
                    // 8 bit point indices
                    parent_point = *(glyf_data + data_offset + 0);
                    child_point = *(glyf_data + data_offset + 1);

                    data_offset += 2;
                }
            }

            // Row major
            f32 mat[4] = {
                1.0f, 0.0f,
                0.0f, 1.0f
            };

            if ((flags & 0x0008) && data_offset + 2 <= entry.length) {
                // WE_HAVE_A_SCALE
                i16 scale_2_14 = (i16)_TT_READ_BE16(glyf_data + data_offset);
                data_offset += 2;

                f32 scale = (f32)scale_2_14 / (f32)(1 << 14);
                mat[0] = scale;
                mat[3] = scale;
            } else if ((flags & 0x0040) && data_offset + 4 <= entry.length) {
                // WE_HAVE_AN_X_AND_Y_SCALE
                i16 xscale_2_14 = (i16)_TT_READ_BE16(glyf_data + data_offset + 0);
                i16 yscale_2_14 = (i16)_TT_READ_BE16(glyf_data + data_offset + 2);
                data_offset += 4;

                mat[0] = (f32)xscale_2_14 / (f32)(1 << 14);
                mat[3] = (f32)yscale_2_14 / (f32)(1 << 14);
            } else if ((flags & 0x0080) && data_offset + 8 <= entry.length) {
                // WE_HAVE_A_TWO_BY_TWO
                i16 m00_2_14 = (i16)_TT_READ_BE16(glyf_data + data_offset + 0);
                i16 m10_2_14 = (i16)_TT_READ_BE16(glyf_data + data_offset + 2);
                i16 m01_2_14 = (i16)_TT_READ_BE16(glyf_data + data_offset + 4);
                i16 m11_2_14 = (i16)_TT_READ_BE16(glyf_data + data_offset + 6);
                data_offset += 8;

                mat[0] = (f32)m00_2_14 / (f32)(1 << 14);
                mat[1] = (f32)m01_2_14 / (f32)(1 << 14);
                mat[2] = (f32)m10_2_14 / (f32)(1 << 14);
                mat[3] = (f32)m11_2_14 / (f32)(1 << 14);
            }

            u32 child_start_index = glyph->num_points;
            if (!_tt_glyph_add_points(file, info, glyph, child_glyph_index)) {
                return false;
            }

            if (parent_point >= 0 && child_point >= 0) {
                warn_emit("TTF compound point aligning unsupported");
            }

            f32 x_offset = (f32)raw_x_offset;
            f32 y_offset = (f32)raw_y_offset;
            if (scale_offset) {
                x_offset = mat[0] * (f32)raw_x_offset + mat[1] * (f32)raw_y_offset;
                y_offset = mat[2] * (f32)raw_x_offset + mat[3] * (f32)raw_y_offset;
            }

            for (u32 i = child_start_index; i < glyph->num_points; i++) {
                f32 x = (f32)glyph->points[i].x;
                f32 y = (f32)glyph->points[i].y;

                glyph->points[i].x = (i16)(mat[0] * x + mat[1] * y + x_offset);
                glyph->points[i].y = (i16)(mat[2] * x + mat[3] * y + y_offset);
            }

            more = (flags & 0x0020) == 0x0020;
        }
    }

    return true;
}

tt_glyph_data tt_glyph_data_from_index(
    mem_arena* arena, string8 file,
    tt_font_info* info, u32 glyph_index
) {
    if (info == NULL || !info->initialized) { goto fail; }

    _tt_glyf_entry entry = _tt_find_glyf_entry(file, info, glyph_index);
    if (entry.length < 10) { goto fail; }

    u32 offset = info->glyf.offset + entry.offset;

    tt_glyph_data glyph = { 
        .x_min = (i16)_TT_READ_BE16(file.str + offset + 2),
        .y_min = (i16)_TT_READ_BE16(file.str + offset + 4),
        .x_max = (i16)_TT_READ_BE16(file.str + offset + 6),
        .y_max = (i16)_TT_READ_BE16(file.str + offset + 8),
    };

    mem_arena_temp scratch = arena_scratch_get(&arena, 1);

    // Here, I add in all the implied bezier points and duplicate the contour
    // start/end point. As such, this should be a sufficient upper bound
    u32 max_points = info->max_glyph_points * 2 + info->max_glyph_contours;
    _tt_temp_glyph temp_glyph = {
        .max_points = max_points,
        .flags = PUSH_ARRAY(scratch.arena, tt_point_flag, max_points),
        .points = PUSH_ARRAY(scratch.arena, v2_i16, max_points),
    };

    if (!_tt_glyph_add_points(file, info, &temp_glyph, glyph_index)) {
        error_emit("Failed to parse TTF glyph");
        arena_scratch_release(scratch);

        goto fail;
    }

    glyph.num_segments = temp_glyph.num_segments;
    glyph.num_points = temp_glyph.num_points;

    glyph.flags = PUSH_ARRAY_NZ(arena, tt_point_flag, glyph.num_points);
    glyph.points = PUSH_ARRAY_NZ(arena, v2_i16, glyph.num_points);

    memcpy(glyph.flags, temp_glyph.flags, sizeof(tt_point_flag) * glyph.num_points);
    memcpy(glyph.points, temp_glyph.points, sizeof(v2_i16) * glyph.num_points);

    arena_scratch_release(scratch);

    return glyph;

fail:
    return (tt_glyph_data){ 0 };
}

tt_glyph_data tt_glyph_data_from_codepoint(
    mem_arena* arena, string8 file,
    tt_font_info* info, u32 codepoint
) {
    u32 index = tt_glyph_index(file, info, codepoint);
    return tt_glyph_data_from_index(arena, file, info, index);
}

u32 tt_glyph_index(string8 file, tt_font_info* info, u32 codepoint) {
    if (info == NULL || !info->initialized) { return 0; }

    u8* subtable = file.str + info->cmap_offset;

    u32 out = 0;

    switch (info->cmap_format) {
        case 0: {
            if (codepoint > 0xff) { return 0; }

            out = subtable[6 + codepoint];
        } break;

        case 4: {
            if (codepoint > 0xffff) { return 0; }
            
            u16 length = _TT_READ_BE16(subtable + 2);
            u16 seg_count = _TT_READ_BE16(subtable + 6) / 2;

            i32 seg = -1;

            if (info->cmap_sorted) {
                // Sorted cmap
                i32 low = 0;
                i32 high = (i32)seg_count - 1;

                while (low <= high) {
                    i32 mid = low + (high - low + 1) / 2;

                    u16 end_code = _TT_READ_BE16(subtable + 14 + mid * 2);
                    u16 start_code = _TT_READ_BE16(subtable + 16 + 2 * seg_count + mid * 2);

                    if (codepoint > end_code) {
                        low = mid + 1;
                    } else if (codepoint < start_code) {
                        high = mid - 1;
                    } else {
                        seg = mid;
                        break;
                    }
                }
            } else {
                // Unsorted cmap
                for (u32 i = 0; i < seg_count; i++) {
                    u16 end_code = _TT_READ_BE16(subtable + 14 + i * 2);
                    u16 start_code = _TT_READ_BE16(subtable + 16 + 2 * seg_count + i * 2);

                    if (start_code <= codepoint && codepoint <= end_code) {
                        seg = (i32)i;
                        break;
                    }
                }
            }

            if (seg == -1) { return 0; }

            i16 id_delta = (i16)_TT_READ_BE16(subtable + 16 + 4 * seg_count + seg * 2);
            u16 id_range_offset = _TT_READ_BE16(subtable + 16 + 6 * seg_count + seg * 2);

            if (id_range_offset == 0) {
                out = (u16)((u16)codepoint + id_delta);
            } else {
                u16 start_code = _TT_READ_BE16(subtable + 16 + 2 * seg_count + seg * 2);
                u32 num_glyph_ids = (length - (16 + 8 * seg_count)) / 2;

                u32 glyph_id_index = (
                    (id_range_offset / 2) -
                    (u32)(seg_count - seg) +
                    (codepoint - start_code)
                );

                if (glyph_id_index >= num_glyph_ids) {
                    return 0; 
                }

                u16 index = _TT_READ_BE16(subtable + 16 + 8 * seg_count + glyph_id_index * 2);

                if (index == 0) {
                    out = index;
                } else {
                    out = (u16)(index + id_delta);
                }
            }
        } break;

        case 6: {
            if (codepoint > 0xffff) { return 0; }

            u16 first_code = _TT_READ_BE16(subtable + 6);
            u16 entry_count = _TT_READ_BE16(subtable + 8);
            u32 index = codepoint - first_code;

            if (index >= entry_count) {
                return 0;
            }

            out = _TT_READ_BE16(subtable + 10 + index * 2);
        } break;

        case 12:
        case 13: {
            u32 num_groups = _TT_READ_BE32(subtable + 12);
            i64 group = -1;

            u32 group_offset = 0, start_code = 0;

            if (info->cmap_sorted) {
                // Sorted cmap
                i64 low = 0;
                i64 high = num_groups - 1;

                while (low <= high) {
                    i64 mid = low + (high - low + 1) / 2;

                    group_offset = 16 + 12 * (u32)mid;
                    start_code = _TT_READ_BE32(subtable + group_offset);
                    u32 end_code = _TT_READ_BE32(subtable + group_offset + 4);

                    if (codepoint > end_code) {
                        low = mid + 1;
                    } else if (codepoint < start_code) {
                        high = mid - 1;
                    } else {
                        group = mid;
                        break;
                    }
                }
            } else {
                // Unsorted cmap
                for (u32 i = 0; i < num_groups; i++) {
                    group_offset = 16 + 12 * i;
                    start_code = _TT_READ_BE32(subtable + group_offset);
                    u32 end_code = _TT_READ_BE32(subtable + group_offset + 4);

                    if (start_code <= codepoint && codepoint <= end_code) {
                        group = i;
                        break;
                    }
                }
            }

            if (group == -1) { return 0; }

            u32 start_index = _TT_READ_BE32(subtable + group_offset + 8);
            if (info->cmap_format == 12) {
                out = start_index + codepoint - start_code;
            } else {
                out = start_index;
            }
        } break;

        default: {
            return 0;
        } break;
    }

    if (out < info->num_glyphs) {
        return out;
    }

    return 0;
}

u32 _tt_calc_checksum(string8 file, u32 offset, u32 len) {
    u32 sum = 0;

    for (u32 i = 0; i < len / 4; i++) {
        sum += _TT_READ_BE32(file.str + offset + i * 4);
    }

    u32 left_over = 0;
    for (u32 i = 0; i < len % 4; i++) {
        left_over += (u32)file.str[offset + (len / 4) * 4 + i] << (3 - i) * 8;
    }

    sum += left_over;

    return sum;
}

b32 _tt_get_validate_table(string8 file, u32 table_tag, tt_font_table* table) {
    u16 num_tables = _TT_READ_BE16(file.str + 4);

    for (u32 i = 0; i < num_tables; i++) {
        u32 record_offset = 12 + 16 * i;
        u32 tag      = _TT_READ_BE32(file.str + record_offset +  0);
        u32 checksum = _TT_READ_BE32(file.str + record_offset +  4);
        u32 offset   = _TT_READ_BE32(file.str + record_offset +  8);
        u32 length   = _TT_READ_BE32(file.str + record_offset + 12);

        if (tag != table_tag) { continue; }
        if (offset + length >= file.size) { return false; }

        u32 real_checksum = _tt_calc_checksum(file, offset, length);

        // Subtracting head checksumAdjust
        if (tag == _TT_TAG("head")) {
            real_checksum -= _TT_READ_BE32(file.str + offset + 8);
        }

        if (checksum != real_checksum) { return false; }

        table->offset = offset;
        table->length = length;

        return true;
    }

    return false;
}

b32 _tt_validate_loca(string8 file, const tt_font_info* info) {
    tt_font_table loca = info->loca;

    if (info->loca_format == 0) {
        // 16-bit offsets
        u32 num_offsets = loca.length / sizeof(u16);

        u32 prev_offset = 0;
        for (u32 i = 0; i < num_offsets; i++) {
            u32 offset = 2 * (u32)_TT_READ_BE16(file.str + loca.offset + i * 2);

            if (offset > info->glyf.length || offset < prev_offset) {
                return false; 
            }

            prev_offset = offset;
        }
    } else {
        // 32-bit offsets
        u32 num_offsets = loca.length / sizeof(u32);

        u32 prev_offset = 0;
        for (u32 i = 0; i < num_offsets; i++) {
            u32 offset = _TT_READ_BE32(file.str + loca.offset + i * 4);

            if (offset > info->glyf.length || offset < prev_offset) {
                return false; 
            }

            prev_offset = offset;

        }
    }

    return true;
}

b32 _tt_find_cmap_subtable(string8 file, tt_font_info* info, tt_font_table cmap) {
    if (cmap.length < 4) { return false; }

    u8* cmap_data = file.str + cmap.offset;
    u16 num_subtables = _TT_READ_BE16(cmap_data + 2);

    // Supported encoding types
    // Only looking for Unicode
    // Lower indices are preferred (looking for larger ranges of codepoints)
    u32 encoding_types[] = {
        (0 << 16) |  4,
        (3 << 16) | 10,
        (0 << 16) |  3,
        (0 << 16) |  6,
        (3 << 16) |  1,
        (1 << 16) |  0,
    };
    u32 num_encoding_types = sizeof(encoding_types) / sizeof(encoding_types[0]);

    u32 selected_encoding_index = UINT32_MAX;
    u32 selected_subtable_offset = 0;

    if (cmap.length < 4 + 8 * num_subtables) { return false; }

    for (u32 i = 0; i < num_subtables; i++) {
        u32 subtable_offset = 4 + 8 * i;
        u16 platform_id = _TT_READ_BE16(cmap_data + subtable_offset + 0);
        u16 encoding_id = _TT_READ_BE16(cmap_data + subtable_offset + 2);
        u32 offset      = _TT_READ_BE32(cmap_data + subtable_offset + 4);

        if (offset > cmap.length) { return false; }

        u32 encoding_index = UINT32_MAX;
        for (u32 j = 0; j < num_encoding_types; j++) {
            if ((((u32)platform_id << 16) | encoding_id) == encoding_types[j]) {
                encoding_index = j;
                break;
            }
        }

        // Preferring smaller indices
        if (encoding_index < selected_encoding_index) {
            selected_encoding_index = encoding_index;
            selected_subtable_offset = offset;
        }
    }

    if (selected_encoding_index == UINT32_MAX) {
        return false;
    }

    u32 cmap_offset = cmap.offset + selected_subtable_offset;
    u32 local_offset = selected_subtable_offset;

    if (cmap.length < local_offset + 2) {
        return false; 
    }
    
    u16 format = _TT_READ_BE16(file.str + cmap_offset);

    info->cmap_sorted = true;

    // Validating format, length, and indices within subtable
    // Not parsing any glyph indices here
    switch (format) {
        case 0: {
            u32 required_len = 6 + 256;

            if (cmap.length < local_offset + required_len) {
                return false;
            }

            u16 length = _TT_READ_BE16(file.str + cmap_offset + 2);

            if (length < required_len) {
                return false;
            }
        } break;

        case 4: {
            if (cmap.length < local_offset + 20) {
                return false;
            }

            u16 length = _TT_READ_BE16(file.str + cmap_offset + 2);
            u16 seg_count = _TT_READ_BE16(file.str + cmap_offset + 6) / 2;
            u32 min_length = 20 + (u32)seg_count * 4 * sizeof(u16);

            if (length < min_length || cmap.length < length) {
                return false;
            }

            u16 prev_end = 0;
            for (u32 i = 0; i < seg_count; i++) {
                u16 end_code = _TT_READ_BE16(file.str + cmap_offset + 14 + i * 2);
                u16 start_code = _TT_READ_BE16(
                    file.str + cmap_offset + 16 + seg_count * 2 + i * 2
                );

                if (end_code < start_code) { return false; }
                if (end_code < prev_end) { info->cmap_sorted = false; }

                prev_end = end_code;
            }
        } break;

        case 6: {
            if (cmap.length < local_offset + 10) {
                return false;
            }

            u16 length = _TT_READ_BE16(file.str + cmap_offset + 2);
            u16 entry_count = _TT_READ_BE16(file.str + cmap_offset + 8);
            u16 implied_length = 10 + entry_count * 2;

            if (length < implied_length || cmap.length < local_offset + length) {
                return false;
            }
        } break;

        case 12:
        case 13:{
            if (cmap.length < local_offset + 16) {
                return false;
            }

            u32 length = _TT_READ_BE32(file.str + cmap_offset + 4);
            u32 num_groups = _TT_READ_BE32(file.str + cmap_offset + 12);
            u32 implied_length = 16 + num_groups * 12;

            if (length < implied_length || cmap.length < local_offset + length) {
                return false;
            }

            u32 prev_end = 0;
            for (u32 i = 0; i < num_groups; i++) {
                u32 group_offset = 16 + 12 * i;
                u32 start_code = _TT_READ_BE32(file.str + cmap_offset + group_offset);
                u32 end_code = _TT_READ_BE32(file.str + cmap_offset + group_offset + 4);

                if (end_code < start_code) { return false; }
                if (end_code < prev_end) { info->cmap_sorted = false; }

                prev_end = end_code;
            }
        } break;

        default: {
            return false;
        } break;
    }

    info->cmap_offset = cmap_offset;
    info->cmap_format = format;

    return true;
}

_tt_glyf_entry _tt_find_glyf_entry(string8 file, tt_font_info* info, u32 glyph_index) {
    if (glyph_index >= info->num_glyphs) {
        return (_tt_glyf_entry) { 0 };
    }

    u8* loca = file.str + info->loca.offset;

    u32 offset = 0;
    u32 next_offset = 0;

    if (info->loca_format == 0) {
        offset = (u32)_TT_READ_BE16(loca + glyph_index * 2) * 2;
        next_offset = (u32)_TT_READ_BE16(loca + (glyph_index + 1) * 2) * 2;
    } else if (info->loca_format == 1) {
        offset = _TT_READ_BE32(loca + glyph_index * 4);
        next_offset = _TT_READ_BE32(loca + (glyph_index + 1) * 4);
    }

    if (next_offset > info->glyf.length) {
        return (_tt_glyf_entry) { 0 };
    }

    return (_tt_glyf_entry) {
        .offset = offset,
        .length = next_offset - offset
    };
}

