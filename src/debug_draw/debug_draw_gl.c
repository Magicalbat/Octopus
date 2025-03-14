
#define _DD_MAX_TEXT_INSTANCES 1024
#define _DD_BASE_FONT_SIZE 24

typedef struct {
    rectf rect;
    rectf uv_rect;
} _dd_text_instance_data;

typedef struct {
    mem_arena* arena;

    struct {
        vec2f uv_scale;
        tt_render_glyph_info glyph_infos[DD_NUM_CODEPOINTS];

        _dd_text_instance_data* data;

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
    _dd_context.arena = arena_create(KiB(256), KiB(64), false);

    _dd_context.text.data = ARENA_PUSH_ARRAY(
        _dd_context.arena, _dd_text_instance_data, _DD_MAX_TEXT_INSTANCES
    );

    // Initializing text stuff
    {
        mem_arena_temp scratch = arena_scratch_get(NULL, 0);

        string8 font_file = plat_file_read(scratch.arena, font_path);
        tt_font_info font_info = { 0 };

        tt_init_font(font_file, &font_info);

        u32* codepoints = ARENA_PUSH_ARRAY(scratch.arena, u32, DD_NUM_CODEPOINTS);
        for (u32 cp = DD_FIRST_CODEPOINT; cp <= DD_LAST_CODEPOINT; cp++) {
            codepoints[cp - DD_FIRST_CODEPOINT] = cp;
        }

        tt_bitmap font_bitmap = tt_render_font_atlas(
            scratch.arena,
            &font_info,
            codepoints, 
            _dd_context.text.glyph_infos,
            DD_NUM_CODEPOINTS,
            TT_RENDER_SDF,
            tt_get_scale_for_height(&font_info, _DD_BASE_FONT_SIZE), 
            3, 256
        );

        _dd_context.text.uv_scale = (vec2f){
            1.0f / (f32)font_bitmap.width,
            1.0f / (f32)font_bitmap.height,
        };

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
            0.0f, 1.0f,
            1.0f, 1.0f,
            0.0f, 0.0f,

            1.0f, 1.0f,
            1.0f, 0.0f,
            0.0f, 0.0f,
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

void debug_draw_set_view(const mat3f* view_mat) {
    glUseProgram(_dd_context.text.shader_program);
    glUniformMatrix3fv(_dd_context.text.view_mat_loc, 1, GL_TRUE, view_mat->m);
}

void debug_draw_text(vec2f pos, f32 font_size, vec4f color, string8 text) {
    u32 num_passes = (u32)((text.size + _DD_MAX_TEXT_INSTANCES - 1) / (u64)_DD_MAX_TEXT_INSTANCES);

    f32 scale = font_size / _DD_BASE_FONT_SIZE;
    vec2f cur_pos = pos;
    u64 text_pos = 0;

    for (u32 i = 0; i < num_passes; i++) {
        u32 instance_index = 0;

        for (u32 j = 0; text_pos < text.size && j < _DD_MAX_TEXT_INSTANCES; j++) {
            u32 cp = text.str[text_pos++];

            if (cp == '\n') {
                // Not sure if this is the typographically correct method,
                // but it should work
                cur_pos.y += font_size * 1.5f;
                cur_pos.x = pos.x;
                continue;
            }

            if (cp < DD_FIRST_CODEPOINT || cp > DD_LAST_CODEPOINT) {
                continue;
            }

            tt_render_glyph_info glyph_info = _dd_context.text.glyph_infos[cp - DD_FIRST_CODEPOINT];

            if (cp == ' ') {
                cur_pos.x += glyph_info.x_advance * scale;
                continue;
            }

            rectf rect = {
                cur_pos.x + glyph_info.offset.x * scale,
                cur_pos.y + (glyph_info.offset.y - glyph_info.bitmap_rect.h) * scale,
                glyph_info.bitmap_rect.w * scale,
                glyph_info.bitmap_rect.h * scale
            };

            _dd_context.text.data[instance_index++] = (_dd_text_instance_data){
                .rect = rect,
                .uv_rect = glyph_info.uv_rect
            };

            cur_pos.x += glyph_info.x_advance * scale;
        }

        glBindBuffer(GL_ARRAY_BUFFER, _dd_context.text.instance_buffer);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(_dd_text_instance_data) * instance_index, _dd_context.text.data);

        glBindTexture(GL_TEXTURE_2D, _dd_context.text.texture);
        glActiveTexture(GL_TEXTURE0);

        glUseProgram(_dd_context.text.shader_program);
        glUniform1i(_dd_context.text.texture_loc, 0); 
        glUniform4f(_dd_context.text.col_loc, color.x, color.y, color.z, color.w);

        glBindVertexArray(_dd_context.text.vert_array);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, (GLsizei)instance_index);
    }
}

void debug_draw_textf(vec2f pos, f32 font_size, vec4f color, const char* fmt, ...) {
    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    va_list args = { 0 };
    va_start(args, fmt);

    string8 text = str8_createfv(scratch.arena, fmt, args);

    debug_draw_text(pos, font_size, color, text);

    va_end(args);

    arena_scratch_release(scratch);
}

void debug_draw_circles(vec2f* points, u32 num_points, f32 radius, vec4f color);

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
        vec2 pos = (u_view_mat * vec3(world_pos, 1.)).xy;
        gl_Position = vec4(pos, 0., 1.);
    }
);
static const char* _dd_text_frag_source = GLSL_SOURCE(
    330,

    layout (location = 0) out vec4 out_col;

    uniform sampler2D u_texture;
    uniform vec4 u_col;

    in vec2 uv;

    void main() {
        float dist = texture(u_texture, uv).r;
        float blending = fwidth(dist);
        float alpha = smoothstep(0.53 - blending, 0.53 + blending, dist);
        out_col = vec4(u_col.rgb, u_col.a * alpha);
    }
);

