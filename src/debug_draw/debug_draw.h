
void debug_draw_init(const window* win);
void debug_draw_destroy(void);

void debug_draw_set_view(view2_f32 view);

void debug_draw_circles(v2_f32* points, u32 num_points, f32 radius, v4_f32 color);
void debug_draw_lines(v2_f32* points, u32 num_points, f32 radius, v4_f32 color);

