
typedef struct {
    u32 offset;
    u32 length;
} tt_font_table;

typedef struct {
    b8 initialized;
    b8 valid;

    i16 loca_format;

    u16 cmap_format;
    // Offset of selected cmap subtable, not cmap table itself
    // Offset is from beginning of file
    u32 cmap_offset;

    tt_font_table head, glyf, hmtx, loca;

} tt_font_info;

void tt_font_init(string8 file, tt_font_info* info);

