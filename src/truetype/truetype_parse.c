
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
    info->valid = true;
    return;

invalid:
    info->initialized = false;
    info->valid = false;
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

    u32 selected_subtable_offset = 0;
    u32 selected_encoding_index = UINT32_MAX;

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

    info->cmap_offset = cmap.offset + selected_subtable_offset;
    u16 format = _TT_READ_BE16(file.str + info->cmap_offset);

    if (format != 0 && format != 4 && format != 6 && format != 12 && format != 13) {
        return false;
    }

    info->cmap_format = format;

    return true;
}

