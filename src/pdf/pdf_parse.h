
typedef struct {
    string8 file;

    struct {
        u32 size;
        u64* offsets;
        u32* generations;
    } xref;

    u32 root_obj;
} pdf_parse_context;

pdf_parse_context* pdf_parse_begin(mem_arena* arena, string8 file);

