string8 str8_from_cstr(u8* cstr) {
    if (cstr == NULL) {
        return (string8){ 0 };
    }

    u8* ptr = cstr;
    u64 size = 0;
    while (*(ptr++)) {
        size++;
    }

    return (string8){ cstr, size };
}

string8 str8_copy(mem_arena* arena, string8 str) {
    u8* data = ARENA_PUSH_ARRAY_NZ(arena, u8, str.size);
    memcpy(data, str.str, str.size);
    return (string8){ data, str.size };
}

u8* str8_to_cstr(mem_arena* arena, string8 str) {
    // out is automatically filled with zeros,
    // so I do not have to explicitly set the null terminator
    u8* out = ARENA_PUSH_ARRAY(arena, u8, str.size + 1);
    memcpy(out, str.str, str.size);
    return out;
}

u64 str8_to_u64(string8 str) {
    u64 out = 0;

    for (u64 i = 0; i < str.size; i++) {
        if (str.str[i] < '0' || str.str[i] > '9') {
            break;
        }

        out *= 10;
        out += str.str[i] - '0';
    }

    return out;
}

b32 str8_to_b32(string8 str) {
    return str8_equals(str, STR8_LIT("true"));
}

b32 str8_equals(string8 a, string8 b) {
    if (a.size != b.size) {
        return false;
    }

    for (u64 i = 0; i < a.size; i++) {
        if (a.str[i] != b.str[i]) {
            return false;
        }
    }

    return true;
}

string8 str8_substr(string8 str, u64 start, u64 end) {
    end = MIN(end, str.size);
    start = MIN(start, end);

    return (string8) {
        .str = str.str + start,
        .size = end - start
    };
}

string8 str8_substr_size(string8 str, u64 start, u64 size) {
    return str8_substr(str, start, start + size);
}

u64 str8_index_char(string8 str, u8 c) {
    u64 i = 0;

    for (; i < str.size; i++) {
        if (str.str[i] == c) {
            break;
        }
    }

    return i;
}

string8 str8_createfv(mem_arena* arena, const char* fmt, va_list in_args) {
    va_list args1, args2 = { 0 };
    va_copy(args1, in_args);
    va_copy(args2, in_args);

    string8 out = { 0 };

    i32 size = vsnprintf(NULL, 0, fmt, args1);
    if (size <= 0) {
        goto end;
    }

    mem_arena_temp maybe_temp = arena_temp_begin(arena);

    out.size = (u64)size;
    // +1 is because vsnprintf HAS to add a null terminator
    // Very dumb, but necessary
    out.str = ARENA_PUSH_ARRAY(maybe_temp.arena, u8, size + 1);

    if (size != vsnprintf((char*)out.str, (u64)size + 1, fmt, args2)) {
        arena_temp_end(maybe_temp);
        out = (string8){ 0 };
        goto end;
    }

end:
    va_end(args1);
    va_end(args2);
    return out;
}
string8 str8_createf(mem_arena* arena, const char* fmt, ...) {
    va_list args = { 0 };
    va_start(args, fmt);

    string8 out = str8_createfv(arena, fmt, args);

    va_end(args);
    
    return out;
}

void str8_list_push(mem_arena* arena, string8_list* list, string8 str) {
    if (list == NULL) {
        return;
    }

    if (list->last == NULL || list->last->size == STR8_LIST_BUCKET_SIZE) {
        // Need to create a new bucket
        string8_bucket* bucket = ARENA_PUSH(arena, string8_bucket);

        SLL_PUSH_BACK(list->first, list->last, bucket);
    }

    // Push string onto bucket
    string8_bucket* cur_bucket = list->last;

    cur_bucket->strs[cur_bucket->size++] = str;

    list->size++;
    list->total_length += str.size;
}

string8 str8_concat(mem_arena* arena, const string8_list* list) {
    if (list == NULL) {
        return (string8){ 0 };
    }

    string8 out = {
        .str = ARENA_PUSH_ARRAY(arena, u8, list->total_length),
        .size = list->total_length,
    };
    u64 pos = 0;

    for (string8_bucket* bucket = list->first; bucket != NULL; bucket = bucket->next) {
        for (u32 i = 0; i < bucket->size; i++) {
            memcpy(out.str + pos, bucket->strs[i].str, bucket->strs[i].size);
            pos += bucket->strs[i].size;
        }
    }

    return out;
}

// Based off of the decoders from Chris Wellons and Mr 4th
// https://nullprogram.com/blog/2017/10/06/
// https://git.mr4th.com/mr4th-public/mr4th/src/branch/main/src/base/base_big_functions.c#L696
string_decode utf8_decode(string8 str, u64 offset) {
    static const u8 lengths[] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 3, 3, 4, 0
    };
    static const u8 first_byte_masks[] = {
        0, 0x7f, 0x1f, 0x0f, 0x07
    };
    static const u8 final_shifts[] = {
        0, 18, 12, 6, 0
    };

    string_decode out = {
        // Replacement Character '�'
        .codepoint = 0xfffd,
        .len = 1
    };

    if (offset >= str.size) {
        return out;
    }

    u32 first_byte = str.str[offset];
    u32 len = lengths[first_byte >> 3];

    if (len <= 0 || offset + len > str.size) {
        return out;
    }

    u32 codepoint = (first_byte & first_byte_masks[len]) << 18;
    switch (len) {
        case 4: codepoint |= ((u32)str.str[offset+3] & 0x3f);
        // fallthrough
        case 3: codepoint |= (((u32)str.str[offset+2] & 0x3f) << 6);
        // fallthrough
        case 2: codepoint |= (((u32)str.str[offset+1] & 0x3f) << 12);
        default: break;
    }

    codepoint >>= final_shifts[len];

    out.codepoint = codepoint;
    out.len = len;

    return out;
}

string_decode utf16_decode(string16 str, u64 offset) {
    string_decode out = {
        // Replacement Character '�'
        .codepoint = 0xfffd,
        .len = 1
    };

    if (offset >= str.size) {
        return out;
    }

    u32 first_char = str.str[offset];

    if (first_char < 0xd800 || first_char > 0xdbff) {
        out.codepoint = first_char;
    } else if (offset + 1 < str.size) {
        u32 second_char = str.str[offset + 1];

        // First char was already checked
        if (second_char >= 0xdc00 && second_char <= 0xdfff) {
            out.codepoint = ((first_char - 0xd800) << 10) |
                (second_char - 0xdc00) + 0x10000;
            out.len = 2;
        }
    }

    return out;
}

u32 utf8_encode(u32 codepoint, u8* out) {
    u32 size = 0;

    if (codepoint <= 0x7f) {
        out[0] = (u8)(codepoint & 0xff);

        size = 1;
    } else if (codepoint <= 0x7ff) {
        out[0] = (u8)(0xc0 | (codepoint >> 6));
        out[1] = (u8)(0x80 | (codepoint & 0x3f));

        size = 2;
    } else if (codepoint <= 0xffff) {
        out[0] = (u8)(0xe0 | (codepoint >> 12));
        out[1] = (u8)(0x80 | ((codepoint >> 6) & 0x3f));
        out[2] = (u8)(0x80 | (codepoint & 0x3f));

        size = 3;
    } else if (codepoint <= 0x10fff) {
        out[0] = (u8)(0xf0 | (codepoint >> 18));
        out[1] = (u8)(0x80 | ((codepoint >> 12) & 0x3f));
        out[2] = (u8)(0x80 | ((codepoint >> 6) & 0x3f));
        out[3] = (u8)(0x80 | (codepoint & 0x3f));

        size = 4;
    } else {
        // Replacement Character '�'
        size = utf8_encode(0xfffd, out);
    }

    return size;
}

u32 utf16_encode(u32 codepoint, u16* out) {
    u32 size = 0;

    if (codepoint < 0x10000) {
        out[0] = (u16)codepoint;

        size = 1;
    } else {
        codepoint -= 0x10000;
        
        out[0] = (u16)((codepoint >> 10) + 0xd800);
        out[1] = (u16)((codepoint & 0x3ff) + 0xdc00);

        size = 1;
    }

    return size;
}

string8 str8_from_str16(mem_arena* arena, string16 str, b32 null_terminate) {
    u64 max_size = str.size * 3 + (null_terminate ? 1 : 0);
    u8* out = ARENA_PUSH_ARRAY(arena, u8, max_size);

    u64 out_size = 0;
    u64 offset = 0;

    while (offset < str.size) {
        string_decode decode = utf16_decode(str, offset);

        offset += decode.len;
        
        out_size += utf8_encode(decode.codepoint, out);
    }

    u64 required_chars = out_size + (null_terminate ? 1 : 0);
    u64 unused_chars = max_size - required_chars;
    arena_pop(arena, unused_chars * sizeof(u8));

    return (string8) {
        .str = out,
        .size = out_size
    };
}

string16 str16_from_str8(mem_arena* arena, string8 str, b32 null_terminate) {
    u64 max_size = str.size + (null_terminate ? 1 : 0);
    u16* out = ARENA_PUSH_ARRAY(arena, u16, max_size);

    u64 out_size = 0;
    u64 offset = 0;

    while (offset < str.size) {
        string_decode decode = utf8_decode(str, offset);

        offset += decode.len;

        out_size += utf16_encode(decode.codepoint, out + out_size);
    }

    u64 required_chars = out_size + (null_terminate ? 1 : 0);
    u64 unused_chars = max_size - required_chars;
    arena_pop(arena, unused_chars * sizeof(u16));

    return (string16) {
        .str = out,
        .size = out_size
    };
}

