#include "truetype_parse.h"

// TODO: remove
#include <stdio.h>

#define READ_BE16(mem) ((((u8*)(mem))[0] << 8) | (((u8*)(mem))[1]))
#define READ_BE32(mem) ((((u8*)(mem))[0] << 24) | (((u8*)(mem))[1] << 16) | (((u8*)(mem))[2] << 8) | (((u8*)(mem))[3]))

#define TAG_TO_U32(tag) (((u32)((tag)[0]) << 24) | ((u32)((tag)[1]) << 16) | ((u32)((tag)[2]) << 8) | (u32)((tag)[3]))

u32 _tt_checksum(u8* data, u64 length);
tt_table_info _tt_get_and_validate_table(string8 file, const char* tag_str);

b32 tt_init_font(string8 file, tt_font_info* font_info) {
    if (file.size < 12) {
        return false;
    }

    if (READ_BE32(file.str) != 0x00010000 && READ_BE32(file.str) != TAG_TO_U32("true")) {
        return false;
    }

    // Because of the checksumAdjust in the head table,
    // the checksum of the file must be the number below
    if (_tt_checksum(file.str, file.size) != 0xB1B0AFBA) {
        return false;
    }

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
        return false;
    }

    tt_table_info cmap = _tt_get_and_validate_table(file, "cmap");
    if (cmap.offset == 0) {
        return false;
    }

    // Getting correct cmap subtable
    {
        if (cmap.length < 4) {
            return false;
        }

        u32 num_tables = READ_BE16(file.str + cmap.offset + 2);
        
        // 4 bytes for header, 8 bytes per encoding record
        if (cmap.length < 4 + 8 * num_tables) {
            return false;
        }

        font_info->cmap_subtable_offset = 0;

        for (u32 i = 0; i < num_tables; i++) {
            // 4 bytes for header, 8 bytes per encoding record
            u64 offset = cmap.offset + 4 + 8 * i;

            u16 platform_id = READ_BE16(file.str + offset);
            u16 encoding_id = READ_BE16(file.str + offset + 2);

            if (platform_id == 0 && (encoding_id == 3 || encoding_id == 4)) {
                // I am only supporting Unicode cmaps
                u32 subtable_offset = READ_BE32(file.str + offset + 4);

                if (subtable_offset < cmap.length) {
                    font_info->cmap_subtable_offset = (u64)cmap.offset + subtable_offset;
                    break;
                }
            }
        }

        if (font_info->cmap_subtable_offset == 0) {
            return false;
        }
    }

    tt_table_info head = _tt_get_and_validate_table(file, "head");
    if (head.offset == 0 || head.length != 54) {
        return false;
    }

    font_info->loca_format = READ_BE16(file.str + head.offset + 50);

    // Verifying that all loca offsets are within the glyf table
    {
        tt_table_info loca = font_info->tables.loca;
        tt_table_info glyf = font_info->tables.glyf;

        if (font_info->loca_format == 0) {
            for (u32 i = 0; i < loca.length; i += 2) {
                u32 offset = READ_BE16(file.str + loca.offset + i) * 2;

                if (offset > glyf.length) {
                    return false;
                }
            }
        } else {
            for (u32 i = 0; i < loca.length; i += 4) {
                u32 offset = READ_BE32(file.str + loca.offset + i);

                if (offset > glyf.length) {
                    return false;
                }
            }
        }
    }

    return true;
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

