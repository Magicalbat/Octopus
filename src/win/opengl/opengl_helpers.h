
#define GLSL_SOURCE(version, src) STR8_LIT("#version " #version " core \n" #src)

u32 glh_create_shader(string8 vertex_source, string8 fragment_source);
u32 glh_create_buffer(u32 buffer_type, u64 size, void* data, u32 draw_type);

