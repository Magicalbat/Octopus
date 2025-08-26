#include "base/base.h"
#include "platform/platform.h"
#include "win/win.h"
#include "truetype/truetype.h"
#include "debug_draw/debug_draw.h"

#include "base/base.c"
#include "platform/platform.c"
#include "win/win.c"
#include "truetype/truetype.c"
#include "debug_draw/debug_draw.c"

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

    win_window* win = window_create(perm_arena, 1280, 720, STR8_LIT("Octopus"));
    window_make_current(win);
    window_clear_color(win, (vec4f){ 0.1f, 0.1f, 0.25f, 1.0f });

#ifndef NDEBUG
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(gl_on_error, NULL);
#endif

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    debug_draw_init(win);

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
        .width = (f32)win->width * 0.5f,
        .center = (vec2f){ 200, 200 }
    };
    mat3f view_mat = { 0 };
    mat3f inv_view_mat = { 0 };
    mat3f_from_view(&view_mat, view);
    mat3f_from_inv_view(&inv_view_mat, view);

    u32 max_points = KiB(16);
    u32 num_points = 0;
    vec2f* points = ARENA_PUSH_ARRAY(perm_arena, vec2f, max_points);

    f64 time = 0.0f;
    u64 prev_frame = plat_time_usec();
    while (!win->should_close) {
        error_frame_begin();

        u64 cur_frame = plat_time_usec();
        f32 delta = (f32)(cur_frame - prev_frame) * 1e-6f;
        time += delta;
        prev_frame = cur_frame;

        window_process_events(win);

        /*for (u32 i = 0; i < win->num_pen_samples; i++) {
            win_pen_sample sample = win->pen_samples[i];

            if (sample.flags & WIN_PEN_FLAG_DOWN) {
                vec2f normalized_pos = {
                    (2.0f * sample.pos.x - (f32)win->width) / (f32)win->width,
                    -(2.0f * sample.pos.y - (f32)win->height) / (f32)win->height,
                };
                vec2f world_pos = mat3f_mul_vec2f(&inv_view_mat, normalized_pos);

                points[num_points++] = world_pos;
            }
        }*/

        for (u32 i = 0; i < win->num_touches; i++) {
            for (u32 j = 0; j < win->touches[i].num_samples; j++) {
                vec2f screen_pos = win->touches[i].positions[j];

                vec2f normalized_pos = {
                    (2.0f * screen_pos.x - (f32)win->width) / (f32)win->width,
                    -(2.0f * screen_pos.y - (f32)win->height) / (f32)win->height,
                };
                vec2f world_pos = mat3f_mul_vec2f(&inv_view_mat, normalized_pos);

                points[num_points++] = world_pos;
            }
        }

        window_clear(win);

        debug_draw_set_view(&view);

        debug_draw_circles(points, num_points, 2.0f, (vec4f){ 1, 1, 1, 1 });

        window_swap_buffers(win);

        if (WIN_KEY_DOWN(win, WIN_KEY_SPACE)) {
            plat_sleep_ms((u32)(1000.0f/60.0f));
        }

        {
            string8 err_str = error_frame_end(perm_arena, ERROR_OUTPUT_CONCAT);
            if (err_str.size) {
                printf("\x1b[31mError(s): %.*s\x1b[0m\n", (int)err_str.size, err_str.str);
                return 1;
            }
        }
    }

    debug_draw_destroy();

    window_destroy(win);

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

