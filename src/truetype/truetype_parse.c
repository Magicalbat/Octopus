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

    font_info->tables.loca = _tt_get_and_validate_table(file, "loca");
    font_info->tables.glyf = _tt_get_and_validate_table(file, "glyf");
    font_info->tables.hhea = _tt_get_and_validate_table(file, "hhea");
    font_info->tables.hmtx = _tt_get_and_validate_table(file, "hmtx");

    if ((
        font_info->tables.loca.offset * 
        font_info->tables.glyf.offset * 
        font_info->tables.hhea.offset * 
        font_info->tables.hmtx.offset 
    ) == 0) {
        // One of the tables was not found
        return false;
    }
    
    return true;
}

u32 _tt_checksum(u8* data, u64 length) {
    u32 sum = 0;

    u64 i = 0;
    for (; i < length; i += 4) {
        sum += READ_BE32(data + i);
    }

    if (i != length) {
        // Length is not divisible by 4
        // Have to go back and get the leftover bytes
        i -= 3;

        for (u32 shift = 24; i < length; i++, shift -= 8) {
            sum += data[i] << shift;
        }
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

