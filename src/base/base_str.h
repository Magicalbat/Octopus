#ifndef BASE_STR_H
#define BASE_STR_H

#include "base_defs.h"
#include "base_arena.h"

#include <stdarg.h>

typedef struct {
    u8* str;
    u64 size;
} string8;

#define STR8_LIST_BUCKET_SIZE 16

typedef struct string8_bucket {
    struct string8_bucket* next;

    string8 strs[STR8_LIST_BUCKET_SIZE];
    u32 size;
} string8_bucket;

typedef struct {
    string8_bucket* first;
    string8_bucket* last;

    u64 size;
    u64 total_length;
} string8_list;

#define STR8_LIT(s) (string8){ (u8*)(s), sizeof(s) - 1 }

string8 str8_from_cstr(u8* cstr);
string8 str8_copy(mem_arena* arena, string8 str);
u8* str8_to_cstr(mem_arena* arena, string8 str);

// Will parse as many valid characters at the start of the string
// e.g. str8_to_u64(STR8_LIT("123asdf")) will return 123
u64 str8_to_u64(string8 str);

b32 str8_equals(string8 a, string8 b);

string8 str8_substr(string8 str, u64 start, u64 end);
string8 str8_substr_size(string8 str, u64 start, u64 size);

string8 str8_createfv(mem_arena* arena, const char* fmt, va_list args);
string8 str8_createf(mem_arena* arena, const char* fmt, ...);

void str8_list_push(mem_arena* arena, string8_list* list, string8 str);

string8 str8_concat(mem_arena* arena, const string8_list* list);

#endif // BASE_STR_H

