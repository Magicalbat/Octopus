#include "truetype_parse.h"

// TODO: remove stdio
#include <stdio.h>
#include <string.h>

#define READ_BE16(mem) ((((u8*)(mem))[0] << 8) | (((u8*)(mem))[1]))
#define READ_BE32(mem) ((((u8*)(mem))[0] << 24) | (((u8*)(mem))[1] << 16) | (((u8*)(mem))[2] << 8) | (((u8*)(mem))[3]))

#define TAG_TO_U32(tag) (((u32)((tag)[0]) << 24) | ((u32)((tag)[1]) << 16) | ((u32)((tag)[2]) << 8) | (u32)((tag)[3]))

u32 _tt_checksum(u8* data, u64 length);
tt_table_info _tt_get_and_validate_table(string8 file, const char* tag_str);
// font_info should be initialized before calling
// -1 is returned if the glyph index has no glyph
void _tt_get_glyph_offset(const tt_font_info* font_info, u32 glyph_index, u32* glyph_offset, u32* glyph_length);

void tt_init_font(string8 file, tt_font_info* font_info) {
    if (font_info == NULL) {
        return;
    }

    memset(font_info, 0, sizeof(tt_font_info));

    // A file of zero size should not emit an error
    // because it probably means the file loaded incorrectly
    // which would have already made an error
    if (file.size == 0) {
        return;
    }

    if (file.size < 12) {
        error_emit("Invalid truetype file");
        return;
    }

    if (READ_BE32(file.str) != 0x00010000 && READ_BE32(file.str) != TAG_TO_U32("true")) {
        error_emit("Unsupported truetype format or invalid truetype file");
        return;
    }

    // Because of the checksumAdjust in the head table,
    // the checksum of the file must be the number below
    if (_tt_checksum(file.str, file.size) != 0xB1B0AFBA) {
        error_emit("Invalid truetype file");
        return;
    }

    font_info->file = file;

    // Just defaults; should be overwritten by the maxp values
    font_info->num_glyphs = 0xffff;
    font_info->max_glyph_points = 0xffff;

    tt_table_info maxp = _tt_get_and_validate_table(file, "maxp");
    if (maxp.offset != 0) {
        u32 version = READ_BE32(file.str + maxp.offset);

        if (version == 0x00005000) {
            font_info->num_glyphs = READ_BE16(file.str + maxp.offset + 4);
        } else if (version == 0x00010000) {
            font_info->num_glyphs = READ_BE16(file.str + maxp.offset + 4);

            u16 max_points = READ_BE16(file.str + maxp.offset + 6);
            u16 max_composite_points = READ_BE16(file.str + maxp.offset + 10);

            font_info->max_glyph_points = MAX(max_points, max_composite_points);
        }
    }

    font_info->tables.loca = _tt_get_and_validate_table(file, "loca");
    font_info->tables.glyf = _tt_get_and_validate_table(file, "glyf");
    font_info->tables.hhea = _tt_get_and_validate_table(file, "hhea");
    font_info->tables.hmtx = _tt_get_and_validate_table(file, "hmtx");

    if (
        font_info->tables.loca.offset == 0 ||
        font_info->tables.glyf.offset == 0 ||
        font_info->tables.hhea.offset == 0 ||
        font_info->tables.hmtx.offset == 0
    ) {
        error_emit("Failed to load loca, glyf, hhea, or hmtx font tables");
        return;
    }

    tt_table_info cmap = _tt_get_and_validate_table(file, "cmap");
    if (cmap.offset == 0) {
        error_emit("Failed to load cmap font table");
        return;
    }

    // Getting and validating correct cmap subtable
    {
        if (cmap.length < 4) {
            error_emit("Invalid cmap font table");
            return;
        }

        u32 num_tables = READ_BE16(file.str + cmap.offset + 2);
        
        // 4 bytes for header, 8 bytes per encoding record
        if (cmap.length < 4 + 8 * num_tables) {
            error_emit("Invalid cmap font table");
            return;
        }

        u32 subtable_offset = 0;
        i32 prev_encoding_id = -1;
        for (u32 i = 0; i < num_tables; i++) {
            // 4 bytes for header, 8 bytes per encoding record
            u64 offset = cmap.offset + 4 + 8 * i;

            u16 platform_id = READ_BE16(file.str + offset);
            u16 encoding_id = READ_BE16(file.str + offset + 2);

            // TODO: Add support for other platforms?
            if (platform_id == 0 && (encoding_id == 3 || encoding_id == 4)) {

                // Prefer encoding id of 4 because it supports more codepoints
                if (prev_encoding_id == 4 && encoding_id == 3) {
                    continue;
                }

                prev_encoding_id = encoding_id;

                subtable_offset = READ_BE32(file.str + offset + 4);

                if (subtable_offset < cmap.length) {
                    font_info->cmap_subtable_offset = (u64)cmap.offset + subtable_offset;
                }
            }
        }

        if (font_info->cmap_subtable_offset == 0) {
            error_emit("Unable to find supported cmap font subtable");
            return;
        }

        // Verifying the length of the subtable
        {
            // Format 4 and Format 12 are both at least 16 bytes long
            if (subtable_offset + 16 > cmap.length) {
                error_emit("Invalid cmap font subtable");
                return;
            }

            u32 format = READ_BE16(file.str + font_info->cmap_subtable_offset);
            u32 subtable_length = 0;
            if (format == 4) {
                subtable_length = READ_BE16(file.str + font_info->cmap_subtable_offset + 2);

                u32 seg_count = READ_BE16(file.str + font_info->cmap_subtable_offset + 6) / 2;

                if (subtable_length < 16 + 2 * seg_count * 4) {
                    error_emit("Invalid format 4 cmap font subtable");
                    return;
                }
            } else if (format == 12) {
                subtable_length = READ_BE32(file.str + font_info->cmap_subtable_offset + 4);

                u32 num_groups = READ_BE32(file.str + font_info->cmap_subtable_offset + 12);

                if (subtable_length != 16 + num_groups * 12) {
                    error_emit("Invalid format 12 cmap font subtable");
                    return;
                }
            } else {
                error_emit("Unsupported cmap font subtable");
                return;
            }

            if (subtable_offset + subtable_length > cmap.length) {
                error_emit("Invalid cmap font subtable");
                return;
            }

            font_info->cmap_format = format;
        }
    }

    tt_table_info head = _tt_get_and_validate_table(file, "head");
    if (head.offset == 0 || head.length != 54) {
        error_emit("Failed to load head font table");
        return;
    }

    font_info->loca_format = READ_BE16(file.str + head.offset + 50);
    if (font_info->loca_format != 0 && font_info->loca_format != 1) {
        error_emit("Invalid loca format in font");
        return;
    }

    // Verifying that all loca offsets are within the glyf table
    {
        tt_table_info loca = font_info->tables.loca;
        tt_table_info glyf = font_info->tables.glyf;

        if (font_info->loca_format == 0) {
            for (u32 i = 0; i < loca.length; i += 2) {
                u32 offset = READ_BE16(file.str + loca.offset + i) * 2;

                if (offset > glyf.length) {
                    error_emit("Invalid loca font table");
                    return;
                }
            }
        } else {
            for (u32 i = 0; i < loca.length; i += 4) {
                u32 offset = READ_BE32(file.str + loca.offset + i);

                if (offset > glyf.length) {
                    error_emit("Invalid loca font table");
                    return;
                }
            }
        }
    }

    u32 max_glyph_index = font_info->tables.loca.length / ((font_info->loca_format + 1) * 2) - 1;
    if (max_glyph_index < font_info->num_glyphs) {
        font_info->num_glyphs = max_glyph_index;
    }

    font_info->initialized = true;
}

u32 tt_get_glyph_index(const tt_font_info* font_info, u32 codepoint) {
    if (font_info == NULL || !font_info->initialized) {
        return 0;
    }

    string8 file = font_info->file;
    u8* subtable = file.str + font_info->cmap_subtable_offset;

    if (font_info->cmap_format == 4) {
        if (codepoint > 0xffff) {
            return 0;
        }

        u32 length = READ_BE16(subtable + 2);
        u16 seg_count = READ_BE16(subtable + 6) / 2;
        u32 glyph_id_array_length = length - (16 + seg_count * 8);
        u16 end_code = 0;
        u16 segment = 0;

        for (; segment < seg_count; segment++) {
            end_code = READ_BE16(subtable + 14 + segment * 2);

            if (end_code >= codepoint) {
                break;
            }
        }

        u16 start_code = READ_BE16(subtable + 16 + seg_count * 2 + segment * 2);

        if (start_code > codepoint) {
            return 0;
        }

        i16 id_delta = READ_BE16(subtable + 16 + seg_count * 4 + segment * 2);
        u16 id_range_offset = READ_BE16(subtable + 16 + seg_count * 6 + segment * 2);

        if (id_range_offset != 0) {
            u32 glyph_id_array_index = id_range_offset / 2 + (segment - seg_count) +
                (codepoint - start_code);

            if (glyph_id_array_index >= glyph_id_array_length) {
                return 0;
            }

            u16 glyph_id = READ_BE16(subtable + 16 + seg_count * 8 + glyph_id_array_index * 2);

            if (glyph_id == 0) {
                return 0;
            }

            return (u16)(glyph_id + id_delta);
        } else {
            return (u16)((u16)codepoint + id_delta);
        }
    } else if (font_info->cmap_format == 12) {
        u32 num_groups = READ_BE32(subtable + 12);
        i64 group = -1;

        for (u32 i = 0; i < num_groups; i++) {
            u32 start_code = READ_BE32(subtable + 16 + i * 12);
            u32 end_code = READ_BE32(subtable + 16 + i * 12 + 4);

            if (codepoint >= start_code && codepoint <= end_code) {
                group = i;
                break;
            }
        }

        if (group == -1) {
            return 0;
        }

        u32 start_code = READ_BE32(subtable + 16 + group * 12);
        u32 start_glyph_id = READ_BE32(subtable + 16 + group * 12 + 8);

        return (codepoint - start_code) + start_glyph_id;
    }

    return 0;
}

f32 tt_get_scale(const tt_font_info* font_info, f32 height) {
    if (font_info == NULL || !font_info->initialized) {
        return 0;
    }

    string8 file = font_info->file;
    tt_table_info hhea = font_info->tables.hhea;

    i16 ascent = READ_BE16(file.str + hhea.offset + 4);
    i16 descent = READ_BE16(file.str + hhea.offset + 6);
    i16 font_height = ascent - descent;

    return height / (f32)font_height;
}

typedef union {
    struct {
        u8 on_curve: 1;
        u8 x_short: 1;
        u8 y_short: 1;
        u8 repeat: 1;
        u8 x_same_or_positive: 1;
        u8 y_same_or_positive: 1;
        u8 reserved: 2;
    };
    u8 bits;
} _tt_glyph_flag;

u32 tt_get_glyph_outline(const tt_font_info* font_info, u32 glyph_index, tt_segment* segments) {
    if (font_info == NULL || !font_info->initialized) {
        return 0;
    }

    tt_table_info glyf_table = font_info->tables.glyf;

    u32 glyph_offset = 0;
    u32 glyph_length = 0;
    _tt_get_glyph_offset(font_info, glyph_index, &glyph_offset, &glyph_length);
    if (glyph_length == 0 || glyph_offset + glyph_length > glyf_table.length) {
        return 0;
    }

    u8* glyph_data = font_info->file.str + glyf_table.offset + glyph_offset;

    u32 num_segments = 0;

    i16 num_contours = READ_BE16(glyph_data);
    // Last end_point_of_contour + 1
    u32 num_points = (u32)READ_BE16(glyph_data + 10 + 2 * (num_contours - 1)) + 1;
    u16 instructions_length = READ_BE16(glyph_data + 10 + num_contours * 2);

    if (num_points == 0) {
        return 0;
    }

    if (num_contours == -1) {
        // TODO: support compound glyphs
        return 0;
    } else {
        mem_arena_temp scratch = arena_scratch_get(NULL, 0);

        _tt_glyph_flag* flags = ARENA_PUSH_ARRAY(scratch.arena, _tt_glyph_flag, num_points);
        i16* x_coords = ARENA_PUSH_ARRAY(scratch.arena, i16, num_points);
        i16* y_coords = ARENA_PUSH_ARRAY(scratch.arena, i16, num_points);

        u32 cur_offset = 12 + 2 * num_contours + instructions_length;

        // Parsing flags
        for (u32 i = 0; i < num_points && cur_offset <= glyph_length; i++) {
            flags[i].bits = *(glyph_data + cur_offset++);

            if (flags[i].repeat) {
                u8 to_repeat = *(glyph_data + cur_offset++);

                while (to_repeat--) {
                    i++;
                    flags[i].bits = flags[i-1].bits;
                }
            }
        }

        // Parsing x-coords
        i16 prev_coord = 0;
        i16 cur_coord_diff = 0;
        for (u32 i = 0; i < num_points && cur_offset <= glyph_length; i++) {
            u32 flag_combined = (flags[i].x_short << 1) | flags[i].x_same_or_positive;

            if (cur_offset == glyph_length && flag_combined != 0b01) {
                break;
            }

            switch (flag_combined) {
                case 0b00: {
                    cur_coord_diff = READ_BE16(glyph_data + cur_offset);
                    cur_offset += 2;
                } break;
                case 0b01: {
                    cur_coord_diff = 0;
                } break;
                case 0b10: {
                    cur_coord_diff = -(*(glyph_data + cur_offset++));
                } break;
                case 0b11: {
                    cur_coord_diff = *(glyph_data + cur_offset++);
                } break;
            }

            x_coords[i] = prev_coord + cur_coord_diff;
            prev_coord = x_coords[i];
        }

        // Parsing y-coords
        prev_coord = 0;
        cur_coord_diff = 0;
        for (u32 i = 0; i < num_points && cur_offset <= glyph_length; i++) {
           u32 flag_combined = (flags[i].y_short << 1) | flags[i].y_same_or_positive;

            if (cur_offset == glyph_length && flag_combined != 0b01) {
                break;
            }

            switch (flag_combined) {
                case 0b00: {
                    cur_coord_diff = READ_BE16(glyph_data + cur_offset);
                    cur_offset += 2;
                } break;
                case 0b01: {
                    cur_coord_diff = 0;
                } break;
                case 0b10: {
                    cur_coord_diff = -(*(glyph_data + cur_offset++));
                } break;
                case 0b11: {
                    cur_coord_diff = *(glyph_data + cur_offset++);
                } break;
            }

            y_coords[i] = prev_coord + cur_coord_diff;
            prev_coord = y_coords[i];
        }

        // Converting data to tt_segments
        cur_offset = 10;

        u32 start_index = 0;
        u32 end_index = 0;
        u32 point_offset = 0;
        u32 contour_length = 0;

        for (u32 contour = 0; contour < (u32)num_contours && cur_offset < glyph_length; contour++) {
            end_index = READ_BE16(glyph_data + cur_offset);
            cur_offset += 2;
            contour_length = end_index + 1 - start_index;

            // Ensure the first point is on the curve
            point_offset = 0;
            while (!flags[start_index + point_offset % contour_length].on_curve) {
                printf("asdlfkjasdf\n");
                point_offset++;
            }

            u32 prev_point_index = start_index + point_offset % contour_length;
            vec2f prev_point = { x_coords[prev_point_index], y_coords[prev_point_index] };
            // The +1 is to add the segment that closes the contour
            for (u32 i = 0; i < contour_length + 1; i++) {
                u32 point_index = start_index + (i + point_offset) % contour_length;
                vec2f point = { x_coords[point_index], y_coords[point_index] };

                if (flags[point_index].on_curve) {
                    segments[num_segments++] = (tt_segment){
                        .type = TT_SEGMENT_LINE,
                        .line = (line2f){ prev_point, point }
                    };

                    prev_point = point;
                } else {
                    // Bezier
                    u32 next_point_index = start_index + (i + 1 + point_offset) % contour_length;
                    vec2f next_point = { x_coords[next_point_index], y_coords[next_point_index] };

                    if (flags[next_point_index].on_curve) {
                        i++;

                        segments[num_segments++] = (tt_segment){
                            .type = TT_SEGMENT_QBEZIER,
                            .qbez = (qbezier2f){
                                prev_point, point, next_point
                            }
                        };

                        prev_point = next_point;
                    } else { // Next point off curve
                        // Get midpoint between the two off-curve points
                        vec2f p2 = vec2f_scale(vec2f_add(point, next_point), 0.5f);

                        segments[num_segments++] = (tt_segment){
                            .type = TT_SEGMENT_QBEZIER,
                            .qbez = (qbezier2f){
                                prev_point, point, p2
                            }
                        };

                        prev_point = p2;
                    }
                }
            }

            start_index = end_index + 1;
        }

        arena_scratch_release(scratch);
    }

    return num_segments;
}

u32 _tt_checksum(u8* data, u64 length) {
    u32 sum = 0;

    u64 i = 0;
    for (; i < ALIGN_DOWN_POW2(length, 4); i += 4) {
        sum += READ_BE32(data + i);
    }

    for (u32 shift = 24; i < length; i++, shift -= 8) {
        sum += data[i] << shift;
    }

    return sum;
}

tt_table_info _tt_get_and_validate_table(string8 file, const char* tag_str) {
    u32 num_tables = READ_BE16(file.str + 4);

    // Table directory is 12 bytes
    // Each table record is 16 bytes
    if (12 + 16 * num_tables >= file.size) {
        goto invalid;
    }

    u32 tag = TAG_TO_U32(tag_str);
    u32 table_record_offset = 0;
    i32 table_index = -1;

    for (u32 i = 0; i < num_tables; i++) {
        // Table directory is 12 bytes
        // Each table record is 16 bytes
        table_record_offset = 12 + 16 * i;

        u32 cur_tag = READ_BE32(file.str + table_record_offset);

        if (cur_tag == tag) {
            table_index = i;
            break;
        }
    }

    if (table_index == -1) {
        goto invalid;
    }

    u32 offset = READ_BE32(file.str + table_record_offset + 8);
    u32 length = READ_BE32(file.str + table_record_offset + 12);

    if (offset + length > file.size) {
        goto invalid;
    }

    u32 file_checksum = READ_BE32(file.str + table_record_offset + 4);
    u32 calculated_checksum = _tt_checksum(file.str + offset, length);

    if (tag == TAG_TO_U32("head")) {
        // When calculating the head checksum, you treat checksum_adjust as zero
        // Instead of modifying the file str, I am subtracting it here
        u32 checksum_adjust = READ_BE32(file.str + offset + 8);
        calculated_checksum -= checksum_adjust;
    }

    if (calculated_checksum != file_checksum) {
        goto invalid;
    }

    return (tt_table_info) {
        .offset = offset,
        .length = length
    };

invalid:
    return (tt_table_info){ 0 };
}

void _tt_get_glyph_offset(const tt_font_info* font_info, u32 glyph_index, u32* glyph_offset, u32* glyph_length) {
    if (glyph_index >= font_info->num_glyphs) {
        return;
    }

    string8 file = font_info->file;

    tt_table_info loca = font_info->tables.loca;

    u32 offset1 = 0;
    u32 offset2 = 0;

    if (font_info->loca_format == 0) {
        offset1 = READ_BE16(file.str + loca.offset + glyph_index * 2) * 2;
        offset2 = READ_BE16(file.str + loca.offset + glyph_index * 2 + 2) * 2;
    } else { // loca_format == 1
        offset1 = READ_BE32(file.str + loca.offset + glyph_index * 4);
        offset2 = READ_BE32(file.str + loca.offset + glyph_index * 4 + 4);
    }

    *glyph_offset = offset1;
    *glyph_length = offset2 - offset1;
}

