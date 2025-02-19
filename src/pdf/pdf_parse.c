
// Gets the string inside the << >>
string8 _pdf_get_dict_str(string8 str);

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

    u64 xref_offset = str8_to_u64(str8_substr(file, startxref_loc + startxref_str.size + 1, file.size));
    string8 trailer_body = _pdf_get_dict_str(str8_substr(file, trailer_loc, file.size));
    
    pdf_parse_context* context = ARENA_PUSH(arena, pdf_parse_context);

    context->file = file;

    return context;
}

string8 _pdf_get_dict_str(string8 str) {
    u64 start_index = 0;

    while (!str8_equals(str8_substr(str, start_index, start_index + 2), STR8_LIT("<<")) && start_index < str.size) {
        start_index++;
    }


    u64 end_index = start_index + 2;
    i32 open_dicts = 1;

    while (open_dicts > 0 && end_index < str.size) {
        string8 sub = str8_substr(str, end_index, end_index + 2);

        if (str8_equals(sub, STR8_LIT("<<"))) {
            open_dicts++;
        } else if (str8_equals(sub, STR8_LIT(">>"))) {
            open_dicts--;
        }

        end_index++;
    }

    return str8_substr(str, start_index + 2, end_index - 2);
}

