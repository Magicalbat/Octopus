
void debug_draw_init(void);
void debug_draw_destroy(void);

void debug_draw_set_view(const mat3f* view_mat);
void debug_draw_circles(vec2f* points, u32 num_points, f32 radius, vec4f color);

