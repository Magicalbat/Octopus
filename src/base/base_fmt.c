
typedef enum {
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

typedef enum {
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

    u32 min_length;
    u32 min_decimal;

    u32 flags;
} _fmt_specifier;

u64 _str8_formatv_impl(string8 fmt, va_list args, u8* out, u64 cap) {
}

string8 str8_formatv(mem_arena* arena, string8 fmt, va_list args) {
    return (string8){ 0 };
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
