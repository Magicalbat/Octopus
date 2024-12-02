#include "base_str.h"

#include <string.h>

string8 str8_from_cstr(u8* cstr) {
    if (cstr == NULL) {
        return (string8){ 0 };
    }

    u64 size = 0;
    while (*(cstr++)) {
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

void str8_list_push(mem_arena* arena, string8_list* list, string8 str) {
    string8_node* node = ARENA_PUSH(arena, string8_node);
    node->str = str;

    SLL_PUSH_BACK(list->first, list->last, node);

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

    for (string8_node* node = list->first; node != NULL; node = node->next) {
        memcpy(out.str + pos, node->str.str, node->str.size);
        pos += node->str.size;
    }

    return out;
}

