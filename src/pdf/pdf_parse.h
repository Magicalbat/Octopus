
typedef struct {
    string8 file;

    u32 xref_size;
    u64* xref_offsets;
    u32* generations;

    u32 root_obj;
} pdf_parse_context;

pdf_parse_context* pdf_parse_begin(mem_arena* arena, string8 file);

