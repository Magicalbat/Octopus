#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "base/base.h"
#include "platform/platform.h"

#include "gfx/gfx.h"
#include "gfx/opengl/opengl_defs.h"
#include "gfx/opengl/opengl_helpers.h"

#if defined(PLATFORM_WIN32)
#    define UNICODE
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>
#    include <GL/gl.h>
#elif defined(PLATFORM_LINUX)
#    include <GL/gl.h>
#endif

#include "truetype/truetype.h"

void gl_on_error(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* user_param);

static const char* line_vert_source;
static const char* line_frag_source;

static const char* texture_vert_source;
static const char* sdf_frag_source;


int main(int argc, char** argv) {
    error_frame_begin();

    plat_init();

    u64 seeds[2] = { 0 };
    plat_get_entropy(seeds, sizeof(seeds));
    prng_seed(seeds[0], seeds[1]);

    mem_arena* perm_arena = arena_create(MiB(64), KiB(264), true);

    char* font_path = argc > 1 ? argv[1] : "res/Envy Code R.ttf";
    string8 font_file = plat_file_read(perm_arena, str8_from_cstr((u8*)font_path));
    tt_font_info font = { 0 };

    tt_init_font(font_file, &font);

    //f32 scale = tt_get_scale(&font, 24.0f);
    tt_segment* segments = ARENA_PUSH_ARRAY(perm_arena, tt_segment, font.max_glyph_points);
    u32 prev_codepoint = 0;
    u32 codepoint = 'R';

    gfx_window* win = gfx_win_create(perm_arena, 1280, 720, STR8_LIT("Octopus"));
    gfx_win_make_current(win);
    gfx_win_clear_color(win, (vec4f){ 0.1f, 0.1f, 0.25f, 1.0f });

#ifndef NDEBUG
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(gl_on_error, NULL);
#endif

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    f32 font_size = 36;
    u32 falloff = 3;
    f32 scale = tt_get_scale_for_height(&font, font_size);

    u32 codepoints[127-32] = { 0 };
    for (u32 i = 33; i <= 127; i++) {
        codepoints[i-33] = i;
    }
    //u32 codepoints[] = { ',' };
    u32 num_codepoints = sizeof(codepoints) / sizeof(u32);

    rectf* glyph_rects = ARENA_PUSH_ARRAY(perm_arena, rectf, num_codepoints);

    u64 start_time = plat_time_usec();
    tt_bitmap bitmap = tt_render_font_atlas(perm_arena, &font, codepoints, glyph_rects, num_codepoints, TT_RENDER_MSDF, scale, falloff, 256);
    u64 end_time = plat_time_usec();
    printf("Font took %fms to render\n", (f32)(end_time - start_time) * 1e-3f);

    u32 texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, bitmap.width, bitmap.height, 0, GL_RGB, GL_UNSIGNED_BYTE, bitmap.data);
    glGenerateMipmap(GL_TEXTURE_2D);

    u32 line_shader = glh_create_shader(line_vert_source, line_frag_source);
    glUseProgram(line_shader);
    u32 ls_mat_loc = glGetUniformLocation(line_shader, "u_view_mat");
    u32 ls_col_loc = glGetUniformLocation(line_shader, "u_col");

    u32 sdf_shader = glh_create_shader(texture_vert_source, sdf_frag_source);
    glUseProgram(sdf_shader);
    u32 sdf_mat_loc = glGetUniformLocation(sdf_shader, "u_view_mat");
    u32 sdf_texture_loc = glGetUniformLocation(sdf_shader, "u_texture");
    u32 sdf_mode_loc = glGetUniformLocation(sdf_shader, "u_mode");

    u32 sdf_mode = 0;

    u32 sdf_vertex_array = 0;
    glGenVertexArrays(1, &sdf_vertex_array);
    glBindVertexArray(sdf_vertex_array);

    rectf sdf_rect = { -(f32)bitmap.width * 7, -(f32)bitmap.height * 5, bitmap.width * 5, bitmap.height * 5 };
    f32 sdf_verts[] = {
        sdf_rect.x, sdf_rect.y,   0.0f, 0.0f,
        sdf_rect.x + sdf_rect.w, sdf_rect.y,   1.0f, 0.0f,
        sdf_rect.x, sdf_rect.y + sdf_rect.h,   0.0f, 1.0f,

        sdf_rect.x, sdf_rect.y + sdf_rect.h,   0.0f, 1.0f,
        sdf_rect.x + sdf_rect.w, sdf_rect.y,   1.0f, 0.0f,
        sdf_rect.x + sdf_rect.w, sdf_rect.y + sdf_rect.h,   1.0f, 1.0f,
    };

    u32 sdf_vertex_buffer = glh_create_buffer(GL_ARRAY_BUFFER, sizeof(sdf_verts), sdf_verts, GL_STATIC_DRAW);

    u32 line_vertex_array = 0;
    glGenVertexArrays(1, &line_vertex_array);
    glBindVertexArray(line_vertex_array);

    u32 max_verts = 0xffff;
    u32 num_verts = 0;
    u32 line_vertex_buffer = glh_create_buffer(GL_ARRAY_BUFFER, max_verts, NULL, GL_STREAM_DRAW);

    // End of setup error frame
    {
        string8 err_str = error_frame_end(perm_arena, ERROR_OUTPUT_CONCAT);
        if (err_str.size) {
            printf("\x1b[31mError(s): %.*s\x1b[0m\n", (int)err_str.size, err_str.str);
            return 1;
        }
    }

    viewf view = {
        .aspect_ratio = (f32)win->width / win->height,
        .width = win->width * 2
    };
    mat3f view_mat = { 0 };
    mat3f inv_view_mat = { 0 };
    mat3f_from_view(&view_mat, view);
    mat3f_from_inv_view(&inv_view_mat, view);

    f32 target_width = view.width;

    vec2f drag_init_mouse_pos = { 0 };

    u64 prev_frame = plat_time_usec();
    while (!win->should_close) {
        error_frame_begin();

        u64 cur_frame = plat_time_usec();
        f32 delta = (f32)(cur_frame - prev_frame) * 1e-6;
        prev_frame = cur_frame;

        gfx_win_process_events(win);

        vec2f mouse_pos = { 0 };
        // Updating view and getting world mouse pos
        {
            view.aspect_ratio = (f32)win->width / win->height;

            vec2f normalized_mouse_pos = (vec2f){
                2.0f * win->mouse_pos.x / win->width - 1.0f,
                -(2.0f * win->mouse_pos.y / win->height - 1.0f)
            };
            mouse_pos = mat3f_mul_vec2f(&inv_view_mat, normalized_mouse_pos);

            if (win->mouse_scroll != 0) {
                target_width = view.width * (1.0f + (-10.0f * win->mouse_scroll * delta));

            }

            if (ABS(view.width - target_width) > 1e-6f) {
                view.width += (target_width - view.width) * (1.0f - expf(-20 * delta));

                // This makes the view zoom toward the mouse
                mat3f_from_inv_view(&inv_view_mat, view);
                vec2f new_mouse_pos = mat3f_mul_vec2f(&inv_view_mat, normalized_mouse_pos);
                view.center = vec2f_add(view.center, vec2f_sub(mouse_pos, new_mouse_pos));
            }

            if (GFX_IS_MOUSE_JUST_DOWN(win, GFX_MB_LEFT)) {
                drag_init_mouse_pos = mouse_pos;
            }
            if (GFX_IS_MOUSE_DOWN(win, GFX_MB_LEFT)) {
                view.center = vec2f_add(view.center, vec2f_sub(drag_init_mouse_pos, mouse_pos));
            }

            mat3f_from_view(&view_mat, view);
            mat3f_from_inv_view(&inv_view_mat, view);
        }

        if (GFX_IS_KEY_JUST_DOWN(win, GFX_KEY_TAB)) {
            sdf_mode += 1;
            sdf_mode %= 2;
        }

        if (GFX_IS_KEY_JUST_DOWN(win, GFX_KEY_1)) {
            glBindTexture(GL_TEXTURE_2D, texture);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
        if (GFX_IS_KEY_JUST_DOWN(win, GFX_KEY_2)) {
            glBindTexture(GL_TEXTURE_2D, texture);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }

        for (gfx_key key = GFX_KEY_A; key <= GFX_KEY_Z; key++) {
            if (GFX_IS_KEY_JUST_DOWN(win, key)) {
                codepoint = key;
                
                break;
            }
        }

        if (GFX_IS_KEY_DOWN(win, GFX_KEY_UP)) {
            codepoint++;
        }
        if (GFX_IS_KEY_DOWN(win, GFX_KEY_DOWN)) {
            codepoint--;
        }
        if (GFX_IS_KEY_JUST_DOWN(win, GFX_KEY_RIGHT)) {
            codepoint++;
        }
        if (GFX_IS_KEY_JUST_DOWN(win, GFX_KEY_LEFT)) {
            codepoint--;
        }

        if (prev_codepoint != codepoint) {
            u32 glyph_index = tt_get_glyph_index(&font, codepoint);
            u32 num_segments = tt_get_glyph_outline(
                &font, glyph_index, segments, 0,
                (mat2f){ .m = { 1, 0, 0, 1 } },
                (vec2f){ 0, 0 }
            );

            mem_arena_temp scratch = arena_scratch_get(NULL, 0);

            #define BEZ_SUBDIVISIONS 10

            num_verts = 0;
            for (u32 i = 0; i < num_segments; i++) {
                switch (segments[i].type) {
                    case TT_SEGMENT_LINE: {
                        num_verts += 2;
                    } break;
                    case TT_SEGMENT_QBEZIER: {
                        num_verts += BEZ_SUBDIVISIONS * 2;
                    } break;
                }
            }

            vec2f* verts = ARENA_PUSH_ARRAY(scratch.arena, vec2f, num_verts);
            u32 cur_vert = 0;
            
            for (u32 i = 0; i < num_segments; i++) {
                vec2f scale = { 1, -1 };

                switch (segments[i].type) {
                    case TT_SEGMENT_LINE: {
                        verts[cur_vert++] = vec2f_comp_mul(segments[i].line.p0, scale);
                        verts[cur_vert++] = vec2f_comp_mul(segments[i].line.p1, scale);
                    } break;
                    case TT_SEGMENT_QBEZIER: {
                        qbezier2f* qbez = &segments[i].qbez;

                        vec2f c0 = vec2f_add(vec2f_sub(qbez->p2, vec2f_scale(qbez->p1, 2.0f)), qbez->p0);
                        vec2f c1 = vec2f_sub(qbez->p1, qbez->p0);
                        vec2f c2 = qbez->p0;

                        vec2f prev_point = c2;
                        for (u32 j = 1; j <= BEZ_SUBDIVISIONS; j++) {
                            f32 t = (f32)j / BEZ_SUBDIVISIONS;
                            vec2f point = vec2f_add(
                                vec2f_add(
                                    vec2f_scale(c0, t * t),
                                    vec2f_scale(c1, 2.0f * t)
                                ), c2
                            );

                            verts[cur_vert++] = vec2f_comp_mul(prev_point, scale);
                            verts[cur_vert++] = vec2f_comp_mul(point, scale);

                            prev_point = point;
                        }
                    } break;
                }
            }

            glBindVertexArray(line_vertex_array);
            glBindBuffer(GL_ARRAY_BUFFER, line_vertex_buffer);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec2f) * num_verts, verts);

            arena_scratch_release(scratch);
        }

        prev_codepoint = codepoint;

        gfx_win_clear(win);

        glUseProgram(line_shader);
        glUniformMatrix3fv(ls_mat_loc, 1, GL_TRUE, view_mat.m);
        glUniform4f(ls_col_loc, 1.0f, 1.0f, 1.0f, 1.0f);

        glBindVertexArray(line_vertex_array);
        glBindBuffer(GL_ARRAY_BUFFER, line_vertex_buffer);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2f), NULL);

        glDrawArrays(GL_LINES, 0, num_verts);

        glDisableVertexAttribArray(0);

        glBindTexture(GL_TEXTURE_2D, texture);
        glActiveTexture(GL_TEXTURE0);

        glUseProgram(sdf_shader);
        glUniformMatrix3fv(sdf_mat_loc, 1, GL_TRUE, view_mat.m);
        glUniform1i(sdf_texture_loc, 0);
        glUniform1i(sdf_mode_loc, sdf_mode);

        glBindVertexArray(sdf_vertex_array);
        glBindBuffer(GL_ARRAY_BUFFER, sdf_vertex_buffer);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2f) * 2, (void*)(0));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vec2f) * 2, (void*)(sizeof(vec2f)));

        glDrawArrays(GL_TRIANGLES, 0, sizeof(sdf_verts) / sizeof(f32));

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);

        gfx_win_swap_buffers(win);

        {
            string8 err_str = error_frame_end(perm_arena, ERROR_OUTPUT_CONCAT);
            if (err_str.size) {
                printf("\x1b[31mError(s): %.*s\x1b[0m\n", (int)err_str.size, err_str.str);
                return 1;
            }
        }
    }

    glDeleteTextures(1, &texture);
    glDeleteProgram(line_shader);
    glDeleteVertexArrays(1, &line_vertex_array);
    glDeleteBuffers(1, &line_vertex_buffer);

    gfx_win_destroy(win);

    arena_destroy(perm_arena);

    return 0;
}

static const char* line_vert_source = GLSL_SOURCE(
    450,

    layout (location = 0) in vec2 a_pos;

    uniform mat3 u_view_mat;

    void main() {
       vec2 pos = (u_view_mat * vec3(a_pos, 1.0)).xy;
       gl_Position = vec4(pos, 0.0, 1.0);
    }
);

static const char* line_frag_source = GLSL_SOURCE(
    450,

    layout (location = 0) out vec4 out_col;

    uniform vec4 u_col;

    void main() {
        out_col = u_col;
    }
);

static const char* texture_vert_source = GLSL_SOURCE(
    450,

    layout (location = 0) in vec2 a_pos;
    layout (location = 1) in vec2 a_uv;

    uniform mat3 u_view_mat;

    out vec2 uv;

    void main() {
        uv = a_uv;
        vec2 pos = (u_view_mat * vec3(a_pos, 1.0)).xy;
        gl_Position = vec4(pos, 0.0, 1.0);
    }
);

static const char* sdf_frag_source = GLSL_SOURCE(
    450,

    layout (location = 0) out vec4 out_col;

    uniform sampler2D u_texture;
    uniform int u_mode;

    in vec2 uv;

    float median(float r, float g, float b) {
        return max(min(r, g), min(max(r, g), b));
    }

    void main() {
        if (u_mode == 0) {
            out_col = vec4(texture(u_texture, uv).rgb, 1.);
            //float dist = texture(u_texture, uv).r;
            //out_col = vec4(dist, dist, dist, 1);
        } else {
            vec3 msd = texture(u_texture, uv).rgb;
            float dist = median(msd.r, msd.g, msd.b);
            float blending = fwidth(dist);
            float alpha = smoothstep(0.53 - blending, 0.53 + blending, dist);
            out_col = vec4(vec3(1.), alpha);
        }
    }
);


void gl_on_error(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* user_param) {
    UNUSED(source);
    UNUSED(type);
    UNUSED(id);
    UNUSED(length);
    UNUSED(user_param);

    if (severity == GL_DEBUG_SEVERITY_HIGH) {
        error_emitf("OpenGL Error: %s", message);
    } else {
        printf("OpenGL Message: %s\n", message);
    }
}

