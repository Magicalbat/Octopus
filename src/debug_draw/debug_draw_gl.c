
#define _DD_MAX_TEXT_INSTANCES 1024

typedef struct {
    vec4f rect;
    vec4f uv_rect;
} _dd_text_instance_data;

typedef struct {
    mem_arena* arena;

    struct {
        f32 scale;
        tt_render_glyph_info glyph_infos[DD_NUM_CODEPOINTS];

        u32 texture;

        u32 shader_program;
        i32 view_mat_loc;
        i32 texture_loc;
        i32 col_loc;

        u32 vert_array;
        u32 pos_pattern_buffer;
        u32 instance_buffer;
    } text;
} _debug_draw_gl_context;

static const char* _dd_text_vert_source;
static const char* _dd_text_frag_source;

static _debug_draw_gl_context _dd_context;

void debug_draw_init(string8 font_path) {
    _dd_context.arena = arena_create(MiB(1), KiB(64), false);

    // Initializing text stuff
    {
        mem_arena_temp scratch = arena_scratch_get(NULL, 0);

        string8 font_file = plat_file_read(scratch.arena, font_path);
        tt_font_info font_info = { 0 };

        tt_init_font(font_file, &font_info);

        _dd_context.text.scale = tt_get_scale_for_height(&font_info, 1.0f);

        u32* codepoints = ARENA_PUSH_ARRAY(scratch.arena, u32, DD_NUM_CODEPOINTS);

        tt_bitmap font_bitmap = tt_render_font_atlas(
            scratch.arena,
            &font_info,
            codepoints, 
            _dd_context.text.glyph_infos,
            DD_NUM_CODEPOINTS,
            TT_RENDER_SDF,
            _dd_context.text.scale * 18, 
            3, 256
        );

        glGenTextures(1, &_dd_context.text.texture);
        glBindTexture(GL_TEXTURE_2D, _dd_context.text.texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_R8,
            (GLsizei)font_bitmap.width,
            (GLsizei)font_bitmap.height,
            0, GL_RED, GL_UNSIGNED_BYTE,
            font_bitmap.data
        );
        glGenerateMipmap(GL_TEXTURE_2D);

        _dd_context.text.shader_program =
            glh_create_shader(_dd_text_vert_source, _dd_text_frag_source);

        glUseProgram(_dd_context.text.shader_program);

        _dd_context.text.view_mat_loc = glGetUniformLocation(_dd_context.text.shader_program, "u_view_mat");
        _dd_context.text.texture_loc = glGetUniformLocation(_dd_context.text.shader_program, "u_texture");
        _dd_context.text.col_loc = glGetUniformLocation(_dd_context.text.shader_program, "u_col");

        f32 pos_pattern_data[] = { 
            0.0f, 0.0f, 
            1.0f, 0.0f,
            1.0f, 1.0f,

            0.0f, 1.0f,
            1.0f, 0.0f,
            1.0f, 1.0f
        };

        glGenVertexArrays(1, &_dd_context.text.vert_array);
        glGenBuffers(1, &_dd_context.text.pos_pattern_buffer);
        glGenBuffers(1, &_dd_context.text.instance_buffer);

        glBindVertexArray(_dd_context.text.vert_array);

        glBindBuffer(GL_ARRAY_BUFFER, _dd_context.text.pos_pattern_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(pos_pattern_data), pos_pattern_data, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void*)0);

        glBindBuffer(GL_ARRAY_BUFFER, _dd_context.text.instance_buffer);
        glBufferData(GL_ARRAY_BUFFER, _DD_MAX_TEXT_INSTANCES * sizeof(_dd_text_instance_data), NULL, GL_STREAM_DRAW);

        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glVertexAttribDivisor(1, 1);
        glVertexAttribDivisor(2, 1);

        glVertexAttribPointer(
            1, 4, GL_FLOAT, GL_FALSE,
            sizeof(_dd_text_instance_data),
            (void*)offsetof(_dd_text_instance_data, rect)
        );
        glVertexAttribPointer(
            2, 4, GL_FLOAT, GL_FALSE,
            sizeof(_dd_text_instance_data),
            (void*)offsetof(_dd_text_instance_data, uv_rect)
        );

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        arena_scratch_release(scratch);
    }

}

void debug_draw_destroy(void) {
    glDeleteTextures(1, &_dd_context.text.texture);
    glDeleteProgram(_dd_context.text.shader_program);
    glDeleteVertexArrays(1, &_dd_context.text.vert_array);
    glDeleteBuffers(1, &_dd_context.text.pos_pattern_buffer);
    glDeleteBuffers(1, &_dd_context.text.instance_buffer);

    arena_destroy(_dd_context.arena);
}

void debug_set_view(const mat3f* view_mat);

void debug_draw_text(vec2f pos, f32 font_size, string8 text);

void debug_draw_textf(vec2f pos, f32 font_size, char* fmt, ...);

void debug_draw_circles(vec2f* points, u32 num_points, f32 radius);

static const char* _dd_text_vert_source = GLSL_SOURCE(
    330,
    layout (location = 0) in vec2 a_pos_pattern;
    layout (location = 1) in vec4 a_rect;
    layout (location = 2) in vec4 a_uv_rect;

    uniform mat3 u_view_mat;

    out vec2 uv;

    void main() {
        uv = a_uv_rect.xy + a_uv_rect.zw * a_pos_pattern;
        vec2 world_pos = a_rect.xy + a_rect.zw * a_pos_pattern;
        vec2 pos = vec3(world_pos, 1.) * u_view_mat;
        gl_Postiion = vec4(pos, 0., 1.);
    }
);
static const char* _dd_text_frag_source = GLSL_SOURCE(
    330,

    layout (location = 0) out vec4 out_col;

    uniform sampler2D u_texture;
    uniform vec3 u_col;

    in vec2 uv;

    void main() {
        float dist = texture(u_texture, uv).r;
        float blending = fwidth(dist);
        float alpha = smoothstep(0.53 - blending, 0.53 + blending, dist);
        out_col = vec4(u_col, alpha);
    }
);

