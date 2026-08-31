
typedef enum {
    _FMT_VT_NONE = 0,

    _FMT_VT_I8,
    _FMT_VT_I16,
    _FMT_VT_I32,
    _FMT_VT_I64,
    _FMT_VT_U8,
    _FMT_VT_U16,
    _FMT_VT_U32,
    _FMT_VT_U64,
    _FMT_VT_B8,
    _FMT_VT_B32,
    _FMT_VT_F32,
    _FMT_VT_F64,
    _FMT_VT_CSTR,
    _FMT_VT_STR8,
    _FMT_VT_V2_I16,
    _FMT_VT_V2_I32,
    _FMT_VT_V2_F32,
    _FMT_VT_V3_F32,
    _FMT_VT_V4_F32,
    _FMT_VT_POINTER,
} _fmt_value_type;

// Describes how to format the given number
// Only works for numbers
typedef enum {
    _FMT_FT_NONE = 0,

    _FMT_FT_DECIMAL,
    _FMT_FT_LOWER_SCIENTIFIC,
    _FMT_FT_UPPER_SCIENTIFIC,
    _FMT_FT_LOWER_ADAPT,
    _FMT_FT_UPPER_ADAPT,
    _FMT_FT_LOWER_HEX,
    _FMT_FT_UPPER_HEX,
    _FMT_FT_OCTAL,
    _FMT_FT_BINARY,
    _FMT_FT_BOOLEAN,
    _FMT_FT_POINTER,
    _FMT_FT_CODEPOINT,
} _fmt_format_type;

typedef enum {
    _FMT_FLAG_LEFT_JUSTIFY   = 0b000001,
    _FMT_FLAG_CENTER_JUSTIFY = 0b000010,
    _FMT_FLAG_ALWAYS_SIGN    = 0b000100,
    _FMT_FLAG_POSITIVE_SPACE = 0b001000,
    _FMT_FLAG_PAD_ZERO       = 0b010000,
    _FMT_FLAG_ADD_COMMAS     = 0b100000,
} _fmt_flags;

typedef struct {
    _fmt_value_type value_type;
    _fmt_format_type format_type;

    u32 min_width;
    u32 min_decimal;

    u32 flags;

    b32 valid;
} _fmt_specifier;

#define _FMT_TRY_PUSH(c) do {\
    if (pos < cap) { out[pos] = c; }\
    pos++; \
} while (0)

_fmt_value_type _fmt_parse_value_type(string8 type_str) {
    if (type_str.size == 0) { return _FMT_VT_NONE; }

    if (type_str.str[type_str.size-1] == '*') {
        return _FMT_VT_POINTER;
    }

    if (str8_equals(STR8_LIT("i8"), type_str)) {
        return _FMT_VT_I8;
    } else if (str8_equals(STR8_LIT("i16"), type_str)) {
        return _FMT_VT_I16;
    } else if (str8_equals(STR8_LIT("i32"), type_str)) {
        return _FMT_VT_I32;
    } else if (str8_equals(STR8_LIT("i64"), type_str)) {
        return _FMT_VT_I64;
    } else if (str8_equals(STR8_LIT("u8"), type_str)) {
        return _FMT_VT_U8;
    } else if (str8_equals(STR8_LIT("u16"), type_str)) {
        return _FMT_VT_U16;
    } else if (str8_equals(STR8_LIT("u32"), type_str)) {
        return _FMT_VT_U32;
    } else if (str8_equals(STR8_LIT("u64"), type_str)) {
        return _FMT_VT_U64;
    } else if (str8_equals(STR8_LIT("b8"), type_str)) {
        return _FMT_VT_B8;
    } else if (str8_equals(STR8_LIT("b32"), type_str)) {
        return _FMT_VT_B32;
    } else if (str8_equals(STR8_LIT("f32"), type_str)) {
        return _FMT_VT_F32;
    } else if (str8_equals(STR8_LIT("f64"), type_str)) {
        return _FMT_VT_F64;
    } else if (str8_equals(STR8_LIT("cstring"), type_str)) {
        return _FMT_VT_CSTR;
    } else if (str8_equals(STR8_LIT("string8"), type_str)) {
        return _FMT_VT_STR8;
    } else if (str8_equals(STR8_LIT("v2_i16"), type_str)) {
        return _FMT_VT_V2_I16;
    } else if (str8_equals(STR8_LIT("v2_i32"), type_str)) {
        return _FMT_VT_V2_I32;
    } else if (str8_equals(STR8_LIT("v2_f32"), type_str)) {
        return _FMT_VT_V2_F32;
    } else if (str8_equals(STR8_LIT("v3_f32"), type_str)) {
        return _FMT_VT_V3_F32;
    } else if (str8_equals(STR8_LIT("v4_f32"), type_str)) {
        return _FMT_VT_V4_F32;
    }

    return _FMT_VT_POINTER;
}

// Returns true if settings is valid
b32 _fmt_parse_settings(_fmt_specifier* out, string8 settings, va_list args) {
    b8 expecting_decimal_width = false;
    b8 parsing_num = false;
    u32 cur_num = 0;

    for (u64 i = 0; i < settings.size; i++) {
        u8 c = settings.str[i];

        if ('1' <= c && c <= '9') {
            if (!parsing_num) {
                parsing_num = true;
                cur_num = 0;
            }

            cur_num *= 10;
            cur_num += c - '0';
            
            continue;
        } else if (parsing_num) {
            // We were parsing the number, now its done
            parsing_num = false;

            if (expecting_decimal_width) {
                out->min_decimal = cur_num;
            } else {
                out->min_width = cur_num;
            }
        } else if (expecting_decimal_width && c != '*') {
            // We were expecting a decimal width, but none was given
            return false;
        }

        switch (c) {
            case 'd': { out->format_type = _FMT_FT_DECIMAL; } break;
            case 'e': { out->format_type = _FMT_FT_LOWER_SCIENTIFIC; } break;
            case 'E': { out->format_type = _FMT_FT_UPPER_SCIENTIFIC; } break;
            case 'f': { out->format_type = _FMT_FT_LOWER_ADAPT; } break;
            case 'F': { out->format_type = _FMT_FT_UPPER_ADAPT; } break;
            case 'x': { out->format_type = _FMT_FT_LOWER_HEX; } break;
            case 'X': { out->format_type = _FMT_FT_UPPER_HEX; } break;
            case 'o': { out->format_type = _FMT_FT_OCTAL; } break;
            case 'b': { out->format_type = _FMT_FT_BINARY; } break;
            case 'B': { out->format_type = _FMT_FT_BOOLEAN; } break;
            case 'p': { out->format_type = _FMT_FT_POINTER; } break;
            case 'c': { out->format_type = _FMT_FT_CODEPOINT; } break;

            case '-': { out->flags |= _FMT_FLAG_LEFT_JUSTIFY; } break;
            case '|': { out->flags |= _FMT_FLAG_CENTER_JUSTIFY; } break;
            case '+': { out->flags |= _FMT_FLAG_ALWAYS_SIGN; } break;
            case ' ': { out->flags |= _FMT_FLAG_POSITIVE_SPACE; } break;
            case '0': { out->flags |= _FMT_FLAG_PAD_ZERO; } break;
            case '\'': { out->flags |= _FMT_FLAG_ADD_COMMAS; } break;

            case '.': { expecting_decimal_width = true; } break;

            case '*': {
                u32 value = va_arg(args, u32);

                if (expecting_decimal_width) {
                    expecting_decimal_width = false;

                    out->min_decimal = value;
                } else {
                    out->min_width = value;
                }
            } break;

            default: { return false; }
        }
    }

    // Extra check in case number was last thing in the string
    if (parsing_num) {
        if (expecting_decimal_width) {
            out->min_decimal = cur_num;
        } else {
            out->min_width = cur_num;
        }
    }

    return true;
}

_fmt_specifier _fmt_parse_specifier(string8 spec, va_list args) {
    _fmt_specifier out = { 0 };

    if (spec.size <= 2 || spec.str[0] != '{' || spec.str[spec.size-1] != '}') {
        goto invalid;
    }

    spec = str8_substr(spec, 1, spec.size - 1);
    u64 colon_i = str8_find_first(spec, ':');

    string8 type_str = str8_substr(spec, 0, colon_i);
    string8 settings_str = str8_substr(spec, colon_i + 1, spec.size);

    out.value_type = _fmt_parse_value_type(type_str);

    // Initializing default format type
    switch (out.value_type) {
        case _FMT_VT_NONE: goto invalid;

        case _FMT_VT_I8:
        case _FMT_VT_I16:
        case _FMT_VT_I32:
        case _FMT_VT_I64:
        case _FMT_VT_U8:
        case _FMT_VT_U16:
        case _FMT_VT_U32:
        case _FMT_VT_U64:
        case _FMT_VT_V2_I16:
        case _FMT_VT_V2_I32: {
            out.format_type = _FMT_FT_DECIMAL;
        } break;

        case _FMT_VT_B8:
        case _FMT_VT_B32: {
            out.format_type = _FMT_FT_BOOLEAN;
        } break;

        case _FMT_VT_F32:
        case _FMT_VT_F64:
        case _FMT_VT_V2_F32:
        case _FMT_VT_V3_F32:
        case _FMT_VT_V4_F32: {
            out.format_type = _FMT_FT_LOWER_ADAPT;
        } break;

        case _FMT_VT_POINTER: {
            out.format_type = _FMT_FT_POINTER;
        } break;

        // Strings do not get a format type
        case _FMT_VT_CSTR:
        case _FMT_VT_STR8: {
            out.format_type = _FMT_FT_NONE;
        } break;
    }

    if (!_fmt_parse_settings(&out, settings_str, args)) {
        goto invalid;
    }

    out.valid = true;

invalid:
    return out;
}

// Returns the size that the final string would have been
// Can be more or less than the cap
u64 _str8_formatv_impl(string8 fmt, va_list args, u8* out, u64 cap) {
    u64 pos = 0;

    u64 specifier_start = 0;
    b8 in_specifier = false;
    for (u64 i = 0; i < fmt.size; i++) {
        u8 c = fmt.str[i];

        if (in_specifier) {
            if (c == '}') {
                in_specifier = false;

                string8 specifier_str = str8_substr(fmt, specifier_start, i + 1);
                _fmt_specifier specifier = _fmt_parse_specifier(specifier_str, args);

                info_emitf(
                    "%u | VT: %2u, FT: %2u, width: %2u, decimal: %u, flags: 0x%x",
                    specifier.valid,
                    specifier.value_type,
                    specifier.format_type,
                    specifier.min_width,
                    specifier.min_decimal,
                    specifier.flags
                );
            }
        } else {
            if (c == '{' || c == '}') {
                if (i + 1 < fmt.size && fmt.str[i+1] == c) {
                    _FMT_TRY_PUSH(c);
                    i++;
                } else if (c == '{') {
                    in_specifier = true;
                    specifier_start = i;
                }
            } else {
                _FMT_TRY_PUSH(c);
            }
        }
    }
    
    return pos;
}

string8 str8_formatv(mem_arena* arena, string8 fmt, va_list args) {
    va_list args2;
    va_copy(args2, args);

    u64 try_size = fmt.size * 2;

    u8* out_data = PUSH_ARRAY_NZ(arena, u8, try_size);
    u64 required_size = _str8_formatv_impl(fmt, args, out_data, try_size);

    if (required_size < try_size) {
        arena_pop(arena, try_size - required_size);
    } else if (required_size > try_size) {
        arena_pop(arena, try_size);
        out_data = PUSH_ARRAY_NZ(arena, u8, required_size);

        required_size = _str8_formatv_impl(fmt, args2, out_data, required_size);
    }

    va_end(args2);

    return (string8){ 
        .size = required_size,
        .str = out_data,
    };
}

string8 str8_format(mem_arena* arena, string8 fmt, ...) {
    va_list args;

    va_start(args, fmt);

    string8 out = str8_formatv(arena, fmt, args);

    va_end(args);

    return out;
}

string8 str8_formatv_cstr(mem_arena* arena, const char* fmt, va_list args) {
    return str8_formatv(arena, str8_from_cstr((u8*)fmt), args);
}

string8 str8_format_cstr(mem_arena* arena, const char* fmt, ...) {
    va_list args;

    va_start(args, fmt);

    string8 out = str8_formatv(arena, str8_from_cstr((u8*)fmt), args);

    va_end(args);

    return out;
}
