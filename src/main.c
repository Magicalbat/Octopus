
#include "base/base.h"
#include "platform/platform.h"
#include "win/win.h"
#include "debug_draw/debug_draw.h"

#include "base/base.c"
#include "platform/platform.c"
#include "win/win.c"
#include "debug_draw/debug_draw.c"

#define _PB_SIZE 64

typedef struct point_bucket {
    struct point_bucket* next;
    u32 num_points;
    v2_f32 points[_PB_SIZE];
} point_bucket;

typedef struct {
    point_bucket* first_bucket;
    point_bucket* last_bucket;

    u32 total_points;
} point_list;

void gl_on_error(
    GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar* message, const void* user_param
);

v2_f32 screen_to_world(window* win, view2_f32* view, v2_f32 p);

void point_list_push(mem_arena* arena, point_list* pl, v2_f32 p);
v2_f32* point_list_as_arr(mem_arena* arena, point_list* pl);

int main(int argc, char** argv) {
    UNUSED(argc);
    UNUSED(argv);

    log_frame_begin();

    plat_init();

    u64 seeds[2] = { 0 };
    plat_get_entropy(seeds, sizeof(seeds));
    prng_seed(seeds[0], seeds[1]);

    mem_arena* perm_arena = arena_create(MiB(64), KiB(264), ARENA_FLAG_GROWABLE);
    mem_arena* frame_arena = arena_create(MiB(16), KiB(264), 0);

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

    mem_arena* point_arena = arena_create(MiB(4), KiB(64), 0);
    point_list touch_point_list = { 0 };
    i64 cur_point_id = -1;

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

        win_process_events(frame_arena, win);

        for (win_event* e = win->first_event; e != NULL; e = e->next) {
            b32 push = true;
            v2_f32 screen_pos = { 0 };
            i64 touch_id = -1;

            switch (e->kind) {
                case WIN_EVENT_TOUCH_DOWN: {
                    screen_pos = e->touch_down.touch_info.pos;
                    touch_id = (i64)e->touch_down.touch_info.id;;

                    if (cur_point_id < 0) {
                        arena_clear(point_arena);
                        touch_point_list = (point_list){ 0 };

                        cur_point_id = touch_id;
                    }
                } break;

                case WIN_EVENT_TOUCH_MOVE: {
                    screen_pos = e->touch_move.touch_info.pos;
                    touch_id = (i64)e->touch_move.touch_info.id;;
                } break;

                case WIN_EVENT_TOUCH_UP: {
                    screen_pos = e->touch_up.touch_info.pos;
                    touch_id = (i64)e->touch_up.touch_info.id;;

                    if (touch_id == cur_point_id) {
                        point_list_push(
                            point_arena, &touch_point_list,
                            screen_to_world(win, &view, screen_pos)
                        );

                        cur_point_id = -1;
                    }
                } break;

                default: {
                    push = false;
                };
            }

            if (push && touch_id == cur_point_id) {
                point_list_push(
                    point_arena, &touch_point_list,
                    screen_to_world(win, &view, screen_pos)
                );
            }
        }

        // Updating view
        {
            view.center.x += win->cur_scroll.x * view.width * 0.04f;
            view.center.y -= win->cur_scroll.y * view.width * 0.04f;

            view.aspect_ratio = (f32)win->width / (f32)win->height;
            m3_f32_from_view2(&view_mat, view);
            debug_draw_set_view(view);
        }

        win_begin_frame(win);

        v2_f32 test_square[] = {
            (v2_f32){ -100, -100 },
            (v2_f32){ -100,  100 },
            (v2_f32){  100,  100 },
            (v2_f32){  100, -100 },
            (v2_f32){ -100, -100 },
        };
        debug_draw_lines(test_square, 5, 5, (v4_f32){ 1, 1, 1, 1 });

        v2_f32* touch_points = point_list_as_arr(frame_arena, &touch_point_list);
        debug_draw_circles(
            touch_points, touch_point_list.total_points, 
            3, (v4_f32){ 0, 1, 0, 1 }
        );

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

void point_list_push(mem_arena* arena, point_list* pl, v2_f32 p) {
    if (pl->last_bucket != NULL && pl->last_bucket->num_points < _PB_SIZE) {
        point_bucket* bucket = pl->last_bucket;

        bucket->points[bucket->num_points++] = p;
    } else {
        point_bucket* bucket = PUSH_STRUCT(arena, point_bucket);

        bucket->points[bucket->num_points++] = p;

        SLL_PUSH_BACK(pl->first_bucket, pl->last_bucket, bucket);
    }

    pl->total_points++;
}

v2_f32* point_list_as_arr(mem_arena* arena, point_list* pl) {
    v2_f32* out = PUSH_ARRAY(arena, v2_f32, pl->total_points);
    u32 out_index = 0;

    for (point_bucket* bucket = pl->first_bucket; bucket != NULL; bucket = bucket->next) {
        for (u32 i = 0; i < bucket->num_points; i++) {
            if (out_index >= pl->total_points) { return out; }

            out[out_index++] = bucket->points[i];
        }
    }

    return out;
}

