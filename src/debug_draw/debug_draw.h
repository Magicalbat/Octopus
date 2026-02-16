
void debug_draw_init(const window* win);
void debug_draw_destroy(void);

void debug_draw_set_view(const viewf* view);

void debug_draw_circles(vec2f* points, u32 num_points, f32 radius, vec4f color);
void debug_draw_lines(vec2f* points, u32 num_points, f32 radius, vec4f color);

