#ifndef BASE_STR_H
#define BASE_STR_H

#include "base_defs.h"
#include "base_arena.h"

#include <stdarg.h>

typedef struct {
    u8* str;
    u64 size;
} string8;

typedef struct {
    u16* str;
    u64 size;
} string16;

typedef struct {
    u32 codepoint;
    // In characters of the string
    u32 len;
} string_decode;

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
u8* str8_to_cstr(mem_arena* arena, string8 str);
string8 str8_copy(mem_arena* arena, string8 str);

// Will parse as many valid characters at the start of the string
// e.g. str8_to_u64(STR8_LIT("123asdf")) will return 123
u64 str8_to_u64(string8 str);
// Returns true if str is equal to "true" (lowercase),
// false otherwise
b32 str8_to_b32(string8 str);

b32 str8_equals(string8 a, string8 b);

string8 str8_substr(string8 str, u64 start, u64 end);
string8 str8_substr_size(string8 str, u64 start, u64 size);

/* Returns str.size if c is not in str
   Can be used to split a string by a character like so:

   string8 str = STR8_LIT("foo bar baz");

   while (str.size) {
       u64 index = str8_index_char(str, (u8)' ');
       string8 part = str8_substr(str, 0, index);
       str = str8_substr(str, index + 1, str.size);

       printf("%.*s\n", (int)part.size, part.str);
   }
*/
u64 str8_index_char(string8 str, u8 c);

string8 str8_createfv(mem_arena* arena, const char* fmt, va_list args);
string8 str8_createf(mem_arena* arena, const char* fmt, ...);

void str8_list_push(mem_arena* arena, string8_list* list, string8 str);

string8 str8_concat(mem_arena* arena, const string8_list* list);

// Returns the decode output of the first unicode codepoint
// after offset bytes in the utf-8 string
string_decode utf8_decode(string8 str, u64 offset);
// Returns the decode output of the first unicode codepoint
// after offset 16-bit characters in the utf-16 string
string_decode utf16_decode(string16 str, u64 offset);
// Encodes the codepoint to out and returns the length in characters
// out must contain at least four characters
u32 utf8_encode(u32 codepoint, u8* out);
// Encodes the codepoint to out and returns the length in characters
// out must contain at least two characters
u32 utf16_encode(u32 codepoint, u16* out);

// Converts a utf-16 string16 to a utf-8 string8
// Null termination is not counted toward the size of the string
string8 str8_from_str16(mem_arena* arena, string16 str, b32 null_terminate);
// Converts a utf-8 string8 to a utf-16 string16
// Null termination is not counted toward the size of the string
string16 str16_from_str8(mem_arena* arena, string8 str, b32 null_terminate);

#endif // BASE_STR_H

