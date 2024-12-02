#ifndef BASE_STR_H
#define BASE_STR_H

#include "base_defs.h"
#include "base_arena.h"

typedef struct {
    u8* str;
    u64 size;
} string8;

typedef struct string8_node {
    struct string8_node* next;
    string8 str;
} string8_node;

typedef struct {
    string8_node* first;
    string8_node* last;

    u64 size;
    u64 total_length;
} string8_list;

#define STR8_LIT(s) (string8){ (u8*)(s), sizeof(s) - 1 }

string8 str8_from_cstr(u8* cstr);
string8 str8_copy(mem_arena* arena, string8 str);
u8* str8_to_cstr(mem_arena* arena, string8 str);

b32 str8_equals(string8 a, string8 b);

string8 str8_substr(string8 str, u64 start, u64 end);
string8 str8_substr_size(string8 str, u64 start, u64 size);

void str8_list_push(mem_arena* arena, string8_list* list, string8 str);

string8 str8_concat(mem_arena* arena, const string8_list* list);

#endif // BASE_STR_H

