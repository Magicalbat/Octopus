
/*
Custom String Formatting
------------------------

Basic Syntax: "{(type):(format type and format settings)}"
Example: "{u32:04x}"

Use "{{" and "}}" to actually get '{' and '}'

Supported types:
    - Integers: i8, i16, i32, i64, u8, u16, u32, u64
    - Booleans: b8, b32
    - Floats: f32, f64
    - Pointers: (any type followed by *)
    - Strings: cstring (null terminated char*), string8
    - Vectors: v2_i16, v2_i32, v2_f32, v3_f32, v4_f32

Supported format type (Only for number or vector types):
    - d: specifies decimal (base 10) formatting
        - Default for integers (and integer vectors)
    - e/E: specifies scientific notation (E for capital)
    - f/F: specifies shortest representation between d and e (F for capital)
        - Default for floats (and floating point vectors)
    - x/X: specifies hex formatting (H for capital)
    - o: specifies octal formatting
    - b: specifies binary formatting
    - B: specifices boolean formatting
        - Default for boolean types
        - Formats 0 to 'false', anything else to 'true'
    - p: specifies pointer formatting
        - Default for pointers
        - Prints hex address with width sizeof(void*)
    - c: specifies the number shall be treated as a unicode codepoint
        - Must be used with integer types
        - e.g. {u8:u} can generally be used for ascii characters
        - e.g. {u32:u} could be used to print something like U'👍'

Only one format type can be specified at a time, rightmost one takes precedent

Supported format settings:
    - Number or *: specifies a minimum length
        - * specifies a u32 length in the variadic args
            - Must come before the actual value to be formatted in the args
        - e.g. {u32:4} specifies a min length of 4, keeping the numbers 
        - e.g. {u16:05} specifies a min length of 5, padded with 0
        - e.g. {string8:4} will prepend spaces if the string is too short
    - .Number or .*: specifies the minimum length after the decimal
        - .* specifies a u32 length in the variadic args
            - Must come before the actual value to be formatted in the args
        - The overall minimum length will override this
    - -: Left-justify output within width (default is right)
    - |: Center output within width (default is right)
        - Left justification takes precedence over center justification
    - +: All numbers are preceded by their sign
    - (space): Positive numbers are preceded by a space
    - 0: Pads number with zeros based on the specified width
    - ': Adds commas to large numbers where appropriate (i.e. every 3 digits)

Format type and settings can be specified in any order

Note on vectors:
    - Vectors are essentially translated as follows
        - {v2_f32:(formatting)} -> ({v.x:(formatting)}, {v.y:(formatting)})
        - Each component is printed one after another with ", " separators
        - Parentheses go around the whole thing
        - Format specifiers are copied to each component
    - If this is too limiting, just manually format each component as you like
*/

string8 str8_formatv(mem_arena* arena, string8 fmt, va_list args);
string8 str8_format(mem_arena* arena, string8 fmt, ...);

string8 str8_formatv_cstr(mem_arena* arena, const char* fmt, va_list args);
string8 str8_format_cstr(mem_arena* arena, const char* fmt, ...);
