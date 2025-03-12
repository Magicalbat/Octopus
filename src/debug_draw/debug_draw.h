#ifndef NDEBUG

#define DD_FIRST_CODEPOINT ' '
#define DD_LAST_CODEPOINT '~'
#define DD_NUM_CODEPOINTS (DD_LAST_CODEPOINT - DD_FIRST_CODEPOINT + 1)

void debug_draw_init(string8 font_path);
void debug_draw_destroy(void);

void debug_set_view(const mat3f* view_mat);
void debug_draw_text(vec2f pos, f32 font_size, string8 text);
void debug_draw_textf(vec2f pos, f32 font_size, char* fmt, ...);
void debug_draw_circles(vec2f* points, u32 num_points, f32 radius);

#endif 

