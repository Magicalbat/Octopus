
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

typedef struct {
    point_bucket* first_bucket;
    point_bucket* last_bucket;
} point_free_list;

void gl_on_error(
    GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar* message, const void* user_param
);

v2_f32 screen_to_world(window* win, view2_f32* view, v2_f32 p);

// Updates view given scrolling and zooming events
void update_view(window* win, view2_f32* view, m3_f32* view_mat);

void point_list_push(
    mem_arena* arena, point_free_list* pfl,
    point_list* pl, v2_f32 p
);
v2_f32* point_list_as_arr(mem_arena* arena, point_list* pl);
void point_list_clear(point_list* pl, point_free_list* pfl);

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

    point_free_list pfl = { 0 };
    point_list touch_points = { 0 };
    point_list pen_points = { 0 };

    i64 cur_touch_id = -1;
    i64 cur_pen_id = -1;

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

                    if (cur_touch_id < 0) {
                        point_list_clear(&touch_points, &pfl);

                        cur_touch_id = touch_id;
                    }
                } break;

                case WIN_EVENT_TOUCH_UPDATE: {
                    screen_pos = e->touch_update.touch_info.pos;
                    touch_id = (i64)e->touch_update.touch_info.id;;
                } break;

                case WIN_EVENT_TOUCH_UP: {
                    screen_pos = e->touch_up.touch_info.pos;
                    touch_id = (i64)e->touch_up.touch_info.id;;

                    if (touch_id == cur_touch_id) {
                        point_list_push(
                            perm_arena, &pfl, &touch_points,
                            screen_to_world(win, &view, screen_pos)
                        );

                        cur_touch_id = -1;
                    }
                } break;

                default: {
                    push = false;
                };
            }

            if (push && touch_id == cur_touch_id) {
                point_list_push(
                    perm_arena, &pfl, &touch_points,
                    screen_to_world(win, &view, screen_pos)
                );
            }
        }

        for (win_event* e = win->first_event; e != NULL; e = e->next) {
            b32 push = true;
            v2_f32 screen_pos = { 0 };
            i64 pen_id = -1;

            switch (e->kind) {
                case WIN_EVENT_PEN_DOWN: {
                    screen_pos = e->pen_down.pen_info.pos;
                    pen_id = (i64)e->pen_down.pen_info.id;;

                    if (cur_pen_id < 0) {
                        point_list_clear(&pen_points, &pfl);

                        cur_pen_id = pen_id;
                    }
                } break;

                case WIN_EVENT_PEN_UPDATE: {
                    screen_pos = e->pen_update.pen_info.pos;
                    pen_id = (i64)e->pen_update.pen_info.id;;
                } break;

                case WIN_EVENT_PEN_UP: {
                    screen_pos = e->pen_up.pen_info.pos;
                    pen_id = (i64)e->pen_up.pen_info.id;;

                    if (pen_id == cur_pen_id) {
                        point_list_push(
                            perm_arena, &pfl, &pen_points,
                            screen_to_world(win, &view, screen_pos)
                        );

                        cur_pen_id = -1;
                    }
                } break;

                default: {
                    push = false;
                };
            }

            if (push && pen_id == cur_pen_id) {
                point_list_push(
                    perm_arena, &pfl, &pen_points,
                    screen_to_world(win, &view, screen_pos)
                );
            }
        }

        update_view(win, &view, &view_mat);

        win_begin_frame(win);

        v2_f32 test_square[] = {
            (v2_f32){ -100, -100 },
            (v2_f32){ -100,  100 },
            (v2_f32){  100,  100 },
            (v2_f32){  100, -100 },
            (v2_f32){ -100, -100 },
        };
        debug_draw_lines(test_square, 5, 5, (v4_f32){ 1, 1, 1, 1 });

        v2_f32* touch_points_arr = point_list_as_arr(frame_arena, &touch_points);
        debug_draw_circles(
            touch_points_arr, touch_points.total_points, 
            3, (v4_f32){ 0, 1, 0, 1 }
        );

        v2_f32* pen_points_arr = point_list_as_arr(frame_arena, &pen_points);
        debug_draw_circles(
            pen_points_arr, pen_points.total_points, 
            3, (v4_f32){ 1, 0, 0, 1 }
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
        error_emitf("[OpenGL Error] %s", message);
    } else {
        info_emitf("[OpenGL Message] %s", message);
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

void update_view(window* win, view2_f32* view, m3_f32* view_mat) {
    view->center.x += win->cur_scroll.x * view->width * 0.04f;
    view->center.y -= win->cur_scroll.y * view->width * 0.04f;

    if (win->cur_trackpad_zoom != 1.0f) {
        // Zooming such that the mouse stays in the same position
        v2_f32 init_mousepos = screen_to_world(win, view, win->cur_mouse_pos);
        view->width *= win->cur_trackpad_zoom;
        v2_f32 final_mousepos = screen_to_world(win, view, win->cur_mouse_pos);

        v2_f32 diff = v2_f32_sub(final_mousepos, init_mousepos);
        view->center = v2_f32_sub(view->center, diff);
    }

    view->aspect_ratio = (f32)win->width / (f32)win->height;
    m3_f32_from_view2(view_mat, *view);
    debug_draw_set_view(*view);
}

void point_list_push(
    mem_arena* arena, point_free_list* pfl,
    point_list* pl, v2_f32 p
) {
    if (pl->last_bucket != NULL && pl->last_bucket->num_points < _PB_SIZE) {
        point_bucket* bucket = pl->last_bucket;

        bucket->points[bucket->num_points++] = p;
    } else {
        point_bucket* bucket = NULL;

        if (pfl->first_bucket != NULL) {
            bucket = pfl->first_bucket;

            SLL_POP_FRONT(pfl->first_bucket, pfl->last_bucket);
        } else {
            bucket = PUSH_STRUCT(arena, point_bucket);
        }

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

void point_list_clear(point_list* pl, point_free_list* pfl) {
    point_bucket* next = NULL;

    for (point_bucket* bucket = pl->first_bucket; bucket != NULL; bucket = next) {
        next = bucket->next;

        memset(bucket, 0, sizeof(point_bucket));

        SLL_PUSH_BACK(pfl->first_bucket, pfl->last_bucket, bucket);
    }

    memset(pl, 0, sizeof(point_list));
}

