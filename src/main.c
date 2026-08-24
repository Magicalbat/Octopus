
#include "base/base.h"
#include "platform/platform.h"
#include "win/win.h"
#include "debug_draw/debug_draw.h"

#include "base/base.c"
#include "platform/platform.c"
#include "win/win.c"
#include "debug_draw/debug_draw.c"

void gl_on_error(
    GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar* message, const void* user_param
);

v2_f32 screen_to_world(window* win, view2_f32* view, v2_f32 p);

int main(int argc, char** argv) {
    UNUSED(argc);
    UNUSED(argv);

    log_frame_begin();

    plat_init();

    u64 seeds[2] = { 0 };
    plat_get_entropy(seeds, sizeof(seeds));
    prng_seed(seeds[0], seeds[1]);

    mem_arena* perm_arena = arena_create(MiB(64), KiB(264), true);
    mem_arena* frame_arena = arena_create(MiB(16), KiB(264), false);

    win_gfx_backend_init();
    window* win = win_create(perm_arena, 1280, 720, STR8_LIT("Octopus"));
    win_make_current(win);

#ifndef NDEBUG
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(gl_on_error, NULL);
#endif

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    debug_draw_init(win);

    view2_f32 view = {
        .width = (f32)win->width,
        .aspect_ratio = (f32)win->width / (f32)win->height
    };
    m3_f32 view_mat = { 0 };

    // End of setup error frame
    {
        mem_arena_temp scratch = arena_scratch_get(NULL, 0);
        string8 other_logs = log_frame_peek(
            scratch.arena, LOG_INFO | LOG_WARN, LOG_RES_CONCAT, true
        );

        if (other_logs.size) {
            printf("%.*s\n", STR8_FMT(other_logs));
        }

        arena_scratch_release(scratch);

        string8 err_str = log_frame_end(
            perm_arena, LOG_ERROR, LOG_RES_CONCAT, true
        );

        if (err_str.size) {
            printf("\x1b[31m%.*s\x1b[0m\n", STR8_FMT(err_str));
            return 1;
        }
    }

    win->clear_color = (v4_f32){ 0.0f, 0.2f, 0.4f, 1.0f };

    while ((win->flags & WIN_FLAG_SHOULD_CLOSE) == 0) {
        log_frame_begin();

        win_process_events(win);

        view.center.x += win->mouse_scroll.x * view.width * 0.04f;
        view.center.y -= win->mouse_scroll.y * view.width * 0.04f;

        if (win->touchpad_zoom != 1.0f) {
            // Zooming such that the mouse stays in the same position
            v2_f32 init_mousepos = screen_to_world(win, &view, win->mouse_pos);
            view.width *= win->touchpad_zoom;
            v2_f32 final_mousepos = screen_to_world(win, &view, win->mouse_pos);

            v2_f32 diff = v2_f32_sub(final_mousepos, init_mousepos);
            view.center = v2_f32_sub(view.center, diff);
        }

        view.aspect_ratio = (f32)win->width / (f32)win->height;
        m3_f32_from_view2(&view_mat, view);
        debug_draw_set_view(view);

        win_begin_frame(win);

        v2_f32 test_point = { 0.0f, 0.0f };
        debug_draw_circles(&test_point, 1, 10, (v4_f32){ 1, 1, 1, 1 });

        win_end_frame(win);

        arena_clear(frame_arena);

        {
            mem_arena_temp scratch = arena_scratch_get(NULL, 0);

            string8 logs = log_frame_peek(
                scratch.arena, LOG_ALL, LOG_RES_CONCAT, true
            );

            if (logs.size) {
                printf("%.*s\n", STR8_FMT(logs));
            }

            arena_scratch_release(scratch);
        }
    }

    debug_draw_destroy();

    win_destroy(win);

    arena_destroy(perm_arena);

    return 0;
}

void gl_on_error(
    GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar* message, const void* user_param
) {
    UNUSED(source);
    UNUSED(type);
    UNUSED(id);
    UNUSED(length);
    UNUSED(user_param);

    if (severity == GL_DEBUG_SEVERITY_HIGH) {
        error_emitf("OpenGL Error: %s", message);
    } else {
        info_emitf("OpenGL Message: %s", message);
    }
}

v2_f32 screen_to_world(window* win, view2_f32* view, v2_f32 p) {
    p = v2_f32_add(p, (v2_f32){
        -(f32)win->width / 2.0f,
        -(f32)win->height / 2.0f 
    });
    p = v2_f32_scale(p, view->width / (f32)win->width);
    p = v2_f32_add(p, view->center);

    return p;
}

