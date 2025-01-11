#include <stdio.h>
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

static const char* vert_source;
static const char* frag_source;

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
    u32 codepoint = 0x211d;

    gfx_window* win = gfx_win_create(perm_arena, 1280, 720, STR8_LIT("Octopus"));
    gfx_win_make_current(win);
    gfx_win_clear_color(win, (vec4f){ 0.1f, 0.1f, 0.25f, 1.0f });

#ifndef NDEBUG
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(gl_on_error, NULL);
#endif

    u32 shader_program = glh_create_shader(vert_source, frag_source);

    glUseProgram(shader_program);
    u32 shader_mat_loc = glGetUniformLocation(shader_program, "u_view_mat");
    u32 shader_col_loc = glGetUniformLocation(shader_program, "u_col");

    u32 vertex_array = 0;
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    u32 max_verts = 0xffff;
    u32 num_verts = 0;
    u32 vertex_buffer = vertex_buffer = glh_create_buffer(GL_ARRAY_BUFFER, max_verts, NULL, GL_DYNAMIC_DRAW);

    // End of setup error frame
    {
        string8 err_str = error_frame_end(perm_arena, ERROR_OUTPUT_CONCAT);
        if (err_str.size) {
            printf("\x1b[31mError(s): %.*s\x1b[0m\n", (int)err_str.size, err_str.str);
            return 1;
        }
    }

    f32 scale = 1e-3f;

    u64 prev_frame = plat_time_usec();
    while (!win->should_close) {
        error_frame_begin();

        u64 cur_frame = plat_time_usec();
        f32 delta = (f32)(cur_frame - prev_frame) * 1e-6;
        prev_frame = cur_frame;

        gfx_win_process_events(win);

        for (gfx_key key = GFX_KEY_A; key <= GFX_KEY_Z; key++) {
            if (GFX_IS_KEY_JUST_DOWN(win, key)) {
                codepoint = key;
                
                break;
            }
        }

        if (GFX_IS_KEY_JUST_DOWN(win, GFX_KEY_UP)) {
            codepoint++;
        }
        if (GFX_IS_KEY_JUST_DOWN(win, GFX_KEY_DOWN)) {
            codepoint--;
        }

        if (prev_codepoint != codepoint) {
            u32 glyph_index = tt_get_glyph_index(&font, codepoint);
            u32 num_segments = tt_get_glyph_outline(&font, glyph_index, segments);

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
                switch (segments[i].type) {
                    case TT_SEGMENT_LINE: {
                        verts[cur_vert++] = segments[i].line.p0;
                        verts[cur_vert++] = segments[i].line.p1;
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

                            verts[cur_vert++] = prev_point;
                            verts[cur_vert++] = point;

                            prev_point = point;
                        }
                    } break;
                }
            }

            glBindVertexArray(vertex_array);
            glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec2f) * num_verts, verts);

            arena_scratch_release(scratch);
        }

        prev_codepoint = codepoint;

        scale *= 1.0f + (10.0f * win->mouse_scroll * delta);

        f32 view_mat[] = {
            scale, 0.0f, 0.0f,
            0.0f, scale * ((f32)win->width / win->height), 0.0f,
            0.0f, 0.0f, 0.0f
        };

        gfx_win_clear(win);

        glUseProgram(shader_program);
        glUniformMatrix3fv(shader_mat_loc, 1, GL_FALSE, view_mat);
        glUniform4f(shader_col_loc, 1.0f, 1.0f, 1.0f, 1.0f);

        glBindVertexArray(vertex_array);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2f), NULL);

        glDrawArrays(GL_LINES, 0, num_verts);

        glDisableVertexAttribArray(0);

        gfx_win_swap_buffers(win);

        {
            string8 err_str = error_frame_end(perm_arena, ERROR_OUTPUT_CONCAT);
            if (err_str.size) {
                printf("\x1b[31mError(s): %.*s\x1b[0m\n", (int)err_str.size, err_str.str);
                return 1;
            }
        }
    }

    gfx_win_destroy(win);

    arena_destroy(perm_arena);

    return 0;
}

static const char* vert_source = GLSL_SOURCE(
    450,

    layout (location = 0) in vec2 a_pos;

    uniform mat3 u_view_mat;

    void main() {
       vec2 pos = (u_view_mat * vec3(a_pos, 1.0)).xy;
       gl_Position = vec4(pos, 0.0, 1.0);
    }
);
static const char* frag_source = GLSL_SOURCE(
    450,

    layout (location = 0) out vec4 out_col;

    uniform vec4 u_col;

    void main() {
        out_col = u_col;
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

