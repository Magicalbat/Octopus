
#define _TT_READ_BE16(m) (u16)( \
    ((u16)((u8*)(m))[0] <<  8) | \
    ((u16)((u8*)(m))[1]))

#define _TT_READ_BE32(m) (u32)( \
    ((u32)((u8*)(m))[0] << 24) | \
    ((u32)((u8*)(m))[1] << 16) | \
    ((u32)((u8*)(m))[2] <<  8) | \
    ((u32)((u8*)(m))[3]))

#define _TT_TAG(s) _TT_READ_BE32((s))

u32 _tt_calc_checksum(string8 file, u32 offset, u32 len);
// Does not bounds check table records
b32 _tt_get_validate_table(string8 file, u32 table_tag, tt_font_table* table);
// Assumes loca is already validated
b32 _tt_validate_loca(string8 file, const tt_font_info* info);
// Assumes cmap is already validated
b32 _tt_find_cmap_subtable(string8 file, tt_font_info* info, tt_font_table cmap);

u32 _tt_glyph_index(string8 file, tt_font_info* info, u32 codepoint);

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

    if (
        !_tt_get_validate_table(file, _TT_TAG("head"), &info->head) ||
        !_tt_get_validate_table(file, _TT_TAG("glyf"), &info->glyf) ||
        !_tt_get_validate_table(file, _TT_TAG("hmtx"), &info->hmtx) ||
        !_tt_get_validate_table(file, _TT_TAG("loca"), &info->loca) ||
        !_tt_get_validate_table(file, _TT_TAG("cmap"), &cmap) ||
        info->head.length != 54
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

    if (!_tt_find_cmap_subtable(file, info, cmap)) {
        error_emit("Cannot parse TTF (unable to find supported cmap)");
        goto invalid;
    }

    info->initialized = true;

    return;

invalid:
    info->initialized = false;
}

void tt_test_draw_glyph(string8 file, tt_font_info* info, u32 codepoint, v2_f32 translate, v2_f32 scale) {
    f32 units_per_em = (f32)_TT_READ_BE16(file.str + info->head.offset + 18);
    scale = v2_f32_scale(scale, 1.0f / units_per_em);

    u32 glyph_index = _tt_glyph_index(file, info, codepoint);
    u32 offset = info->loca_format == 0 ?
        (u32)_TT_READ_BE16(file.str + info->loca.offset + glyph_index * 2) * 2 :
        _TT_READ_BE32(file.str + info->loca.offset + glyph_index * 4);

    u8* glyf = file.str + info->glyf.offset + offset;

    i16 num_contours = (i16)_TT_READ_BE16(glyf);
    //i16 x_min = (i16)_TT_READ_BE16(glyf + 2);
    //i16 y_min = (i16)_TT_READ_BE16(glyf + 4);
    //i16 x_max = (i16)_TT_READ_BE16(glyf + 6);
    //i16 y_max = (i16)_TT_READ_BE16(glyf + 8);

    if (num_contours < 0) {
        return;
    }

    u32 num_points = _TT_READ_BE16(glyf + 10 + (num_contours - 1) * 2) + 1;

    u16 instruction_length = _TT_READ_BE16(glyf + 10 + num_contours * 2);
    u8* point_data = glyf + 10 + num_contours * 2 + 2 + instruction_length;

    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    u32 num_flags = 0;
    u8* flags = PUSH_ARRAY(scratch.arena, u8, num_points);
    v2_f32* points = PUSH_ARRAY(scratch.arena, v2_f32, num_points + 1);

    while (num_flags < num_points) {
        u8 flag = *(point_data++);
        flags[num_flags++] = flag;

        // REPEAT_FLAG
        if (flag & 0x08) {
            u8 count = *(point_data++);

            while (count--) {
                flags[num_flags++] = flag;
            }
        }
    }

    i16 x = 0;
    for (u32 i = 0; i < num_points; i++) {
        // X_SHORT_VECTOR
        if (flags[i] & 0x02) {
            u8 x_diff = *(point_data++);
            // X_IS_SAME_OR_POSITIVE_X_SHORT_VECTOR 
            x += (flags[i] & 0x10) ? x_diff : -(i16)x_diff;
        } else if ((flags[i] & 0x10) != 0x10) {
            i16 x_diff = (i16)_TT_READ_BE16(point_data);
            point_data += 2;

            x += x_diff;
        }

        points[i].x = x;
    }

    i16 y = 0;
    for (u32 i = 0; i < num_points; i++) {
        // Y_SHORT_VECTOR
        if (flags[i] & 0x04) {
            u8 y_diff = *(point_data++);
            // Y_IS_SAME_OR_POSITIVE_Y_SHORT_VECTOR
            y += (flags[i] & 0x20) ? y_diff : -(i16)y_diff;
        } else if ((flags[i] & 0x20) != 0x20) {
            i16 y_diff = (i16)_TT_READ_BE16(point_data);
            point_data += 2;

            y += y_diff;
        }

        points[i].y = y;
    }

    for (u32 i = 0; i < num_points; i++) {
        points[i] = v2_f32_add(v2_f32_comp_mul(points[i], scale), translate);
    }

    for (u32 c = 0; c < (u32)num_contours; c++) {
        u32 start_point = c == 0 ? 0 : _TT_READ_BE16(glyf + 10 + (c-1) * 2) + 1;
        u32 end_point = _TT_READ_BE16(glyf + 10 + c * 2);

        u32 num_points = end_point - start_point + 1;

        mem_arena_temp temp_arena = arena_temp_begin(scratch.arena);

        u32 num_draw_points = 0;
        v2_f32* draw_points = PUSH_ARRAY(temp_arena.arena, v2_f32, num_points * 5);

        u32 point_offset = 0;
        while (point_offset < num_points && !(flags[start_point + point_offset] & 0x1)) {
            point_offset++;
        }

        if (point_offset == num_points) {
            point_offset = 0;
        } else {
            draw_points[num_draw_points++] = points[start_point + point_offset];
        }

        for (u32 i = 0; i < num_points; i++) {
            u32 p0 = ((i + point_offset + 0) % num_points) + start_point;
            u32 p1 = ((i + point_offset + 1) % num_points) + start_point;
            u32 p2 = ((i + point_offset + 2) % num_points) + start_point;

            u32 on_curve = (u32)(
                ((flags[p0] & 0x1) << 2) |
                ((flags[p1] & 0x1) << 1) |
                ((flags[p2] & 0x1) << 0));

            v2_f32 a = points[p0];
            v2_f32 b = points[p1];
            v2_f32 c = points[p2];

            b32 bez = true;

            switch (on_curve) {
                case 0b110:
                case 0b111: {
                    // Line Segment
                    draw_points[num_draw_points++] = points[p1];
                    bez = false;
                } break;

                case 0b101: {
                    // Bezier
                    // a, b, and c are already correct
                    // i needs to be incremented
                    i++;
                } break;

                case 0b100: {
                    // Bezier
                    // End control point is implied
                    c = v2_f32_scale(v2_f32_add(b, c), 0.5f);
                } break;

                case 0b001: {
                    // Bezier
                    // Start control point is implied
                    a = v2_f32_scale(v2_f32_add(a, b), 0.5f);
                    // i needs to be incremented
                    i++;
                } break;

                case 0b000: {
                    // Bezier
                    // Both start and end are implied
                    a = v2_f32_scale(v2_f32_add(a, b), 0.5f);
                    c = v2_f32_scale(v2_f32_add(b, c), 0.5f);
                } break;

                case 0b010:
                case 0b011: {
                    // This should be impossible
                    // TODO: remove this, just for testing
                    printf("oops %u\n", codepoint);
                } break;
            }

            if (!bez) { continue; }

            for (f32 t = 0.2f; t <= 1.0f; t += 0.2f) {
                v2_f32 m0 = v2_f32_add(a, v2_f32_scale(v2_f32_sub(b, a), t));
                v2_f32 m1 = v2_f32_add(b, v2_f32_scale(v2_f32_sub(c, b), t));
                v2_f32 p = v2_f32_add(m0, v2_f32_scale(v2_f32_sub(m1, m0), t));
                draw_points[num_draw_points++] = p;
            }
        }

        debug_draw_lines(
            draw_points, num_draw_points,
            2.0f, (v4_f32){ 1, 1, 1, 1 }
        );

        arena_temp_end(temp_arena);
    }

    arena_scratch_release(scratch);
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

    if (info->loca_format == 0) { // 16-bit offsets
        u32 num_offsets = loca.length / sizeof(u16);

        for (u32 i = 0; i < num_offsets; i++) {
            u32 offset = 2 * (u32)_TT_READ_BE16(file.str + loca.offset + i * 2);
            if (offset > info->glyf.length) { return false; }
        }
    } else { // 32-bit offsets
        u32 num_offsets = loca.length / sizeof(u32);

        for (u32 i = 0; i < num_offsets; i++) {
            u32 offset = _TT_READ_BE32(file.str + loca.offset + i * 4);
            if (offset > info->glyf.length) { return false; }
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

u32 _tt_glyph_index(string8 file, tt_font_info* info, u32 codepoint) {
    if (info == NULL || !info->initialized) { return 0; }

    u8* subtable = file.str + info->cmap_offset;

    switch (info->cmap_format) {
        case 0: {
            if (codepoint > 0xff) { return 0; }

            return subtable[6 + codepoint];
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
                return (u16)((u16)codepoint + id_delta);
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
                    return index;
                }

                return (u16)(index + id_delta);
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

            return _TT_READ_BE16(subtable + 10 + index * 2);
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
                return start_index + codepoint - start_code;
            } else {
                return start_index;
            }
        } break;

        default: {
            return 0;
        } break;
    }

    return 0;
}

