#include "base/base.h"
#include "platform/platform.h"
#include "gfx/gfx.h"
#include "truetype/truetype.h"
#include "pdf/pdf.h"

#include "base/base.c"
#include "platform/platform.c"
#include "gfx/gfx.c"
#include "truetype/truetype.c"
#include "pdf/pdf.c"

void gl_on_error(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* user_param);

int main(int argc, char** argv) {
    UNUSED(argc);
    UNUSED(argv);

    error_frame_begin();

    plat_init();

    u64 seeds[2] = { 0 };
    plat_get_entropy(seeds, sizeof(seeds));
    prng_seed(seeds[0], seeds[1]);

    mem_arena* perm_arena = arena_create(MiB(64), KiB(264), true);

    string8 pdf_file = plat_file_read(perm_arena, STR8_LIT("res/test.pdf"));

    pdf_parse_begin(perm_arena, pdf_file);

    {
        string8 err_str = error_frame_end(perm_arena, ERROR_OUTPUT_CONCAT);
        if (err_str.size) {
            printf("\x1b[31mError(s): %.*s\x1b[0m\n", (int)err_str.size, err_str.str);
            return 1;
        }
    }

    return 0;

    gfx_window* win = gfx_win_create(perm_arena, 1280, 720, STR8_LIT("Octopus"));
    gfx_win_make_current(win);
    gfx_win_clear_color(win, (vec4f){ 0.1f, 0.1f, 0.25f, 1.0f });

#ifndef NDEBUG
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(gl_on_error, NULL);
#endif

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // End of setup error frame
    {
        string8 err_str = error_frame_end(perm_arena, ERROR_OUTPUT_CONCAT);
        if (err_str.size) {
            printf("\x1b[31mError(s): %.*s\x1b[0m\n", (int)err_str.size, err_str.str);
            return 1;
        }
    }

    viewf view = {
        .aspect_ratio = (f32)win->width / (f32)win->height,
        .width = (f32)win->width
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
        f32 delta = (f32)(cur_frame - prev_frame) * 1e-6f;
        prev_frame = cur_frame;

        gfx_win_process_events(win);

        vec2f mouse_pos = { 0 };
        // Updating view and getting world mouse pos
        {
            view.aspect_ratio = (f32)win->width / (f32)win->height;

            vec2f normalized_mouse_pos = (vec2f){
                2.0f * win->mouse_pos.x / (f32)win->width - 1.0f,
                -(2.0f * win->mouse_pos.y / (f32)win->height - 1.0f)
            };
            mouse_pos = mat3f_mul_vec2f(&inv_view_mat, normalized_mouse_pos);

            if (win->mouse_scroll != 0) {
                target_width = target_width * (1.0f + (-10.0f * (f32)win->mouse_scroll * delta));
            }

            if (ABS(view.width - target_width) > 1e-6f) {
                view.width += (target_width - view.width) * (1.0f - expf(-20 * delta));

                // This makes the view zoom toward the mouse
                mat3f_from_inv_view(&inv_view_mat, view);
                vec2f new_mouse_pos = mat3f_mul_vec2f(&inv_view_mat, normalized_mouse_pos);
                vec2f diff = vec2f_sub(mouse_pos, new_mouse_pos);
                view.center = vec2f_add(view.center, diff);
            }

            if (GFX_IS_MOUSE_JUST_DOWN(win, GFX_MB_RIGHT)) {
                drag_init_mouse_pos = mouse_pos;
            }
            if (GFX_IS_MOUSE_DOWN(win, GFX_MB_RIGHT)) {
                vec2f diff = vec2f_sub(drag_init_mouse_pos, mouse_pos);
                view.center = vec2f_add(view.center, diff);
            }

            mat3f_from_view(&view_mat, view);
            mat3f_from_inv_view(&inv_view_mat, view);
        }

        gfx_win_clear(win);

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

