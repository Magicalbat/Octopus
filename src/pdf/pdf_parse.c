
#define _PDF_IS_WHITESPACE(c) ((c) == '\0' || (c) == '\t' || (c) == '\r' || (c) == '\n' || (c) == '\f' || (c) == ' ')
#define _PDF_IS_EOL(c) ((c) == '\r' || (c) == '\n')
#define _PDF_HEX_TO_NUM(c) ( \
    (c) >= '0' && (c) <= '9' ? (c) - '0' : \
    (c) >= 'a' && (c) <= 'f' ? (c) - 'a' + 10 : \
    (c) >= 'A' && (c) <= 'F' ? (c) - 'A' + 10 : \
    0 )

typedef enum {
    _PDF_OBJ_NULL,
    _PDF_OBJ_BOOL,
    _PDF_OBJ_NUM,
    _PDF_OBJ_LIT_STR,
    _PDF_OBJ_HEX_STR,
    _PDF_OBJ_NAME,
    _PDF_OBJ_ARRAY,
    _PDF_OBJ_DICT,
    _PDF_OBJ_STREAM,
    _PDF_OBJ_REF
} _pdf_obj_type;

typedef struct {
    _pdf_obj_type type;

    // Stores the string contents of the obj
    // This includes any delimiters
    string8 str;

    // Offset of this str relative to the str passed into _pdf_next_obj_str
    u64 offset;
} _pdf_typed_obj_str;

// Increments index to next non-comment character
// Assumes that the character is not in a string or stream
// Can increment past str.size; caller must perform their own bounds checking
void _pdf_str_index_increment(string8 str, u64* index);

// Returns the next object after any whitespace
_pdf_typed_obj_str _pdf_next_obj_str(string8 str, u64 offset);

// name_str should come directly from a _pdf_typed_obj_str
string8 _pdf_parse_name(mem_arena* arena, string8 name_str);

b32 _pdf_next_dict_elem(
    mem_arena* arena, string8 str,
    string8* out_name, _pdf_typed_obj_str* out_elem,
    u64* offset
);

pdf_parse_context* pdf_parse_begin(mem_arena* arena, string8 file) {
    if (file.size <= 7 || 
        !str8_equals(str8_substr(file, 0, 5), STR8_LIT("%PDF-")) ||
        !str8_equals(str8_substr(file, file.size - 6, file.size - 1), STR8_LIT("%%EOF"))
    ) {
        error_emit("Cannot parse PDF: invalid PDF file");
        return NULL;
    }

    static const string8 startxref_str = STR8_LIT("startxref");
    u64 startxref_loc = file.size - 1 - startxref_str.size;
    while (startxref_loc > 0) {
        if (str8_equals(str8_substr_size(file, startxref_loc, startxref_str.size), startxref_str)) {
            break;
        }
        startxref_loc--;
    }

    static const string8 trailer_str = STR8_LIT("trailer");
    u64 trailer_loc = startxref_loc;
    while (trailer_loc > 0) {
        if (str8_equals(str8_substr_size(file, trailer_loc, trailer_str.size), trailer_str)) {
            break;
        }
        trailer_loc--;
    }

    if (startxref_loc == 0 || trailer_loc == 0) {
        error_emit("Cannot parse PDF: invalid PDF file");
        return NULL;
    }

    //u64 xref_offset = str8_to_u64(
    //    str8_substr(file, startxref_loc + startxref_str.size + 1, file.size)
    //);

    _pdf_typed_obj_str trailer_obj = _pdf_next_obj_str(file, trailer_loc + trailer_str.size);
    if (trailer_obj.type != _PDF_OBJ_DICT) {
        error_emit("Cannot parse PDF: invalid trailer obj type");
        return NULL;
    }

    {
        mem_arena_temp scratch = arena_scratch_get(NULL, 0);
        string8 name = { 0 };
        _pdf_typed_obj_str obj = { 0 };
        u64 offset = 0;

        while (_pdf_next_dict_elem(scratch.arena, trailer_obj.str, &name, &obj, &offset)) {
            printf("'%.*s': (%u '%.*s')\n", (int)name.size, name.str, obj.type, (int)obj.str.size, obj.str.str);

            arena_temp_end(scratch);
        }

        arena_scratch_release(scratch);
    }

    pdf_parse_context* context = ARENA_PUSH(arena, pdf_parse_context);

    context->file = file;

    return context;
}

void _pdf_str_index_increment(string8 str, u64* index) {
    (*index)++;

    if (*index < str.size && str.str[*index] == '%') {
        while (*index < str.size && !_PDF_IS_EOL(str.str[*index])) {
            (*index)++;
        }
    }
}

_pdf_typed_obj_str _pdf_next_obj_str(string8 str, u64 offset) {
    while (offset < str.size && _PDF_IS_WHITESPACE(str.str[offset])) {
        _pdf_str_index_increment(str, &offset);
    }

    _pdf_typed_obj_str obj = {
        .type = _PDF_OBJ_NULL,
        .offset = offset
    };

    if (offset == str.size) {
        return obj;
    }

    switch (str.str[offset]) {
        case 't': {
            string8 true_str = str8_substr(str, offset, offset + 4);
            if (str8_equals(true_str, STR8_LIT("true"))) {
                obj.type = _PDF_OBJ_BOOL;
                obj.str = true_str; 
            }
        } break;
        case 'f': {
            string8 false_str = str8_substr(str, offset, offset + 5);
            if (str8_equals(false_str, STR8_LIT("false"))) {
                obj.type = _PDF_OBJ_BOOL;
                obj.str = false_str;
            }
        } break;
        case 'n': {
            string8 null_str = str8_substr(str, offset, offset + 4);
            if (str8_equals(null_str, STR8_LIT("null"))) {
                // type is already null
                obj.str = null_str;
            }
        } break;

        case '-':
        case '+':
        case '.':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
            u64 i = offset;
            u32 num_decimals = 0;
            b32 just_num = true;

            if (str.str[i] == '-' || str.str[i] == '+') {
                just_num = false;
                i++;
            }

            while (i < str.size && num_decimals <= 1 &&  (
                str.str[i] == '.' || 
                (str.str[i] >= '0' && str.str[i] <= '9')
            )) {
                i++;
                num_decimals += str.str[i] == '.';
            }

            obj.type = _PDF_OBJ_NUM;
            obj.str = str8_substr(str, offset, i);

            if (just_num && num_decimals == 0) {

                // This is a positive integer,
                // so it could be the first number in an object reference
                // which is two numbers and 'R' separated by whitespace 

                i++;
                while (i < str.size && _PDF_IS_WHITESPACE(str.str[i])) {
                    _pdf_str_index_increment(str, &i);
                }

                u64 second_num_start = i;
                while (i < str.size && str.str[i] >= '0' && str.str[i] <= '9') {
                    _pdf_str_index_increment(str, &i);
                }

                if (i != second_num_start) {
                    // There is a second num, check for R
                    while (i < str.size && _PDF_IS_WHITESPACE(str.str[i])) {
                        _pdf_str_index_increment(str, &i);
                    }

                    if (i < str.size && str.str[i] == 'R') {
                        obj.type = _PDF_OBJ_REF;
                        obj.str = str8_substr(str, offset, i + 1);
                    }
                }
            }
        } break;

        case '(': {
            u64 i = offset + 1;
            i32 open_parens = 1;

            while (i < str.size && open_parens >= 1) {
                if (str.str[i] == '(') {
                    open_parens++;
                } else if (str.str[i] == ')') {
                    open_parens--;
                } else if (str.str[i] == '\\') {
                    // Move past escaped character
                    // This should still work with the octal character codes (\xxx)
                    // because this is not parsing the string yet
                    i++;
                }

                i++;
            }

            if (open_parens == 0) { // null otherwise
                obj.type = _PDF_OBJ_LIT_STR;
                obj.str = str8_substr(str, offset, i);
            }
        } break;

        case '<': {
            if (offset + 1 < str.size && str.str[offset + 1] == '<') {
                // Opening of a dict or stream (which begins with a dict)

                u64 i = offset + 2;
                i32 open_obj_brackets = 1;

                while (i < str.size && open_obj_brackets >= 1) {
                    if (str8_equals(str8_substr(str, i, i + 2), STR8_LIT("<<"))) {
                        open_obj_brackets++;
                    } else if (str8_equals(str8_substr(str, i, i + 2), STR8_LIT(">>"))) {
                        open_obj_brackets--;
                    }

                    _pdf_str_index_increment(str, &i);
                }

                if (open_obj_brackets == 0) {
                    obj.type = _PDF_OBJ_DICT;
                    obj.str = str8_substr(str, offset, i + 1);
                }

                i++;

                while (i < str.size && _PDF_IS_WHITESPACE(str.str[i])) {
                    _pdf_str_index_increment(str, &i);
                }

                if (str8_equals(str8_substr(str, i, i + 6), STR8_LIT("stream"))) {
                    i += 6;

                    b32 endstream_found = false;

                    while (
                        i < str.size && !endstream_found
                    ) {
                        endstream_found |= str8_equals(str8_substr(str, i, i + 9), STR8_LIT("endstream"));
                        i++;
                    }

                    if (endstream_found) {
                        obj.type = _PDF_OBJ_STREAM;
                        obj.str = str8_substr(str, offset, i + 8);
                    }
                }
            } else {
                // Opening of a hex string

                u64 i = offset + 1;

                while (i < str.size && str.str[i] != '>') {
                    _pdf_str_index_increment(str, &i);
                }

                if (i != str.size) { // null otherwise
                    obj.type = _PDF_OBJ_HEX_STR;
                    obj.str = str8_substr(str, offset, i + 1);
                }
            }
        } break;

        case '/': {
            u64 i = offset + 1;

            while (i < str.size && str.str[i] >= '!' && str.str[i] <= '~' && str.str[i] != '%') {
                i++;
            }

            obj.type = _PDF_OBJ_NAME;
            obj.str = str8_substr(str, offset, i);
        } break;

        case '[': {
            u64 i = offset + 1;
            i32 open_brackets = 1;

            while (i < str.size && open_brackets >= 1) {
                if (str.str[i] == '[') {
                    open_brackets++;
                } else if (str.str[i] == ']') {
                    open_brackets--;
                }

                _pdf_str_index_increment(str, &i);
            }

            if (open_brackets == 0) { // null otherwise
                obj.type = _PDF_OBJ_ARRAY;
                obj.str = str8_substr(str, offset, i);
            }
        } break;

        default: break;
    }

    return obj;
}

string8 _pdf_parse_name(mem_arena* arena, string8 name_str) {
    if (name_str.size <= 0) {
        return (string8){ 0 };
    }

    if (name_str.str[0] != '/') {
        return (string8){ 0 };
    }

    name_str = str8_substr(name_str, 1, name_str.size);

    u64 out_size = 0;
    u8* out_data = ARENA_PUSH_ARRAY(arena, u8, name_str.size);

    for (u64 i = 0; i < name_str.size; i++) {
        u8 c = name_str.str[i];
        
        if (c == '#') {
            c = 0;
            u32 j = 1;
            for (; i + j < name_str.size && j <= 2; j++) {
                c <<= 4;
                c |= _PDF_HEX_TO_NUM(name_str.str[i + j]);
            }

            i += j - 1;
        }

        out_data[out_size++] = c;
    }

    string8 out = (string8){
        .size = out_size,
        .str = out_data
    };

    return out;
}

b32 _pdf_next_dict_elem(mem_arena* arena, string8 str, string8* out_name, _pdf_typed_obj_str* out_elem, u64* offset) {
    while (*offset < str.size && _PDF_IS_WHITESPACE(str.str[*offset])) {
        _pdf_str_index_increment(str, offset);
    }

    string8 next_2 = str8_substr(str, *offset, *offset + 2);

    if (str8_equals(next_2, STR8_LIT("<<"))) {
        *offset += 2;

        while (*offset < str.size && _PDF_IS_WHITESPACE(str.str[*offset])) {
            _pdf_str_index_increment(str, offset);
        }
    }

    if (str8_equals(next_2, STR8_LIT(">>"))) {
        return false;
    }

    _pdf_typed_obj_str name_obj = _pdf_next_obj_str(str, *offset);

    if (name_obj.type != _PDF_OBJ_NAME) {
        return false;
    }

    *offset = name_obj.offset + name_obj.str.size;

    *out_name = _pdf_parse_name(arena, name_obj.str);
    *out_elem = _pdf_next_obj_str(str, *offset);

    *offset = out_elem->offset + out_elem->str.size;

    return true;
}

