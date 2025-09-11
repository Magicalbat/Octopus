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

#define TARGET_FPS 120

void gl_on_error(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* user_param);

vec2f screen_to_world(vec2f screen_pos, const win_window* win, const mat3f inv_view_mat);

void one_euro_filter(vec2f* raw_points, u64* times_us, u32 num_points, u32 offset, vec2f* out_points, f32 freq_cmin, f32 beta);

#define RAW_OFFSET_Y 0

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
    vec2f* raw_points = ARENA_PUSH_ARRAY(perm_arena, vec2f, max_points);
    vec2f* points = ARENA_PUSH_ARRAY(perm_arena, vec2f, max_points);
    u64* point_times_us = ARENA_PUSH_ARRAY(perm_arena, u64, max_points);

    u32 max_strokes = 1024;
    u32 num_strokes = 0;
    u32* stroke_ends = ARENA_PUSH_ARRAY(perm_arena, u32, max_strokes);

    i64 motion0_touch_id = -1;
    vec2f motion0_touch_pos = { 0 };
    i64 motion1_touch_id = -1;
    f32 motion01_dist = 0.0f;

    f32 freq_cmin = 0.1f;
    f32 beta = 10.0f;

    f64 time = 0.0f;
    u64 prev_frame_start_us = plat_time_usec();
    while (!win->should_close) {
        error_frame_begin();

        u64 frame_start_us = plat_time_usec();
        f32 delta = (f32)(frame_start_us - prev_frame_start_us) * 1e-6f;
        time += delta;
        prev_frame_start_us = frame_start_us;

        window_process_events(win);

        // Establish primary motion touch
        if (win->num_touches > 0 && motion0_touch_id == -1) {
            motion0_touch_id = win->touches[0].id;
            motion0_touch_pos = screen_to_world(win->touches[0].cur_pos, win, inv_view_mat);
        }

        // Establish secondary motion touch
        if (win->num_touches > 1 && motion0_touch_id != -1 && motion1_touch_id == -1) {
            u32 index = 0;
            while (win->touches[index].id == motion0_touch_id) {
                index++;
            }

            motion1_touch_id = win->touches[index].id;
            vec2f touch1_pos = screen_to_world(win->touches[index].cur_pos, win, inv_view_mat);

            motion01_dist = vec2f_dist(motion0_touch_pos, touch1_pos);
        }

        if (motion0_touch_id >= 0) {
            i64 index0 = -1;
            for (u32 i = 0; i < win->num_touches; i++) {
                if (win->touches[i].id == motion0_touch_id) {
                    index0 = i;
                    break;
                }
            }

            if (index0 == -1) {
                motion0_touch_id = -1;
                motion1_touch_id = -1;
            } else {
                vec2f cur_pos0 = screen_to_world(win->touches[index0].cur_pos, win, inv_view_mat);

                vec2f diff = vec2f_sub(motion0_touch_pos, cur_pos0);

                view.center = vec2f_add(view.center, diff);

                if (motion1_touch_id >= 0) {
                    i64 index1 = -1;
                    for (u32 i = 0; i < win->num_touches; i++) {
                        if (win->touches[i].id == motion1_touch_id) {
                            index1 = i;
                            break;
                        }
                    }

                    if (index1 == -1) {
                        motion1_touch_id = -1;
                    } else {
                        vec2f cur_pos1 = screen_to_world(win->touches[index1].cur_pos, win, inv_view_mat);
                        f32 cur_dist = vec2f_dist(cur_pos0, cur_pos1);

                        view.width *= motion01_dist / cur_dist;
                    }
                }
            }
        }
        
        mat3f_from_view(&view_mat, view);
        mat3f_from_inv_view(&inv_view_mat, view);

        for (u32 i = 0; i < win->num_pen_samples; i++) {
            win_pen_sample sample = win->pen_samples[i];

            if (sample.flags & WIN_PEN_FLAG_JUST_DOWN) {
                num_strokes++;

                if (num_strokes >= 2) {
                    stroke_ends[num_strokes-1] = stroke_ends[num_strokes-2];
                }
            }

            if (sample.flags & WIN_PEN_FLAG_DOWN) {
                u32 index = num_points;

                point_times_us[index] = sample.time_us;

                vec2f cur_point_raw = screen_to_world(sample.pos, win, inv_view_mat);
                raw_points[index] = (vec2f){
                    cur_point_raw.x,
                    cur_point_raw.y + RAW_OFFSET_Y
                };

                u32 stroke_start = num_strokes <= 1 ? 0 : stroke_ends[num_strokes-2];

                one_euro_filter(
                    raw_points + stroke_start,
                    point_times_us + stroke_start,
                    num_points + 1 - stroke_start,
                    num_points - stroke_start,
                    points + stroke_start,
                    freq_cmin, beta / view.width
                );

                num_points++;
                stroke_ends[num_strokes-1]++;
            }
        }

        f32 prev_freq = freq_cmin;
        f32 prev_beta = beta;

        if (WIN_KEY_DOWN(win, WIN_KEY_ARROW_DOWN))
            freq_cmin *= 0.9f;
        if (WIN_KEY_DOWN(win, WIN_KEY_ARROW_UP))
            freq_cmin *= 1.1f;

        if (WIN_KEY_DOWN(win, WIN_KEY_S))
            beta *= 0.9f;
        if (WIN_KEY_DOWN(win, WIN_KEY_W))
            beta *= 1.1f;

        if (WIN_KEY_JUST_DOWN(win, WIN_KEY_C)) {
            printf("freq_cmin: %f, beta: %f\n\n", freq_cmin, beta);
        }

        if (freq_cmin != prev_freq || prev_beta != beta) {
            u32 stroke_start = num_strokes == 1 ? 0 : stroke_ends[num_strokes-2];

            one_euro_filter(
                raw_points + stroke_start,
                point_times_us + stroke_start,
                num_points + 1 - stroke_start,
                num_points - stroke_start,
                points + stroke_start,
                freq_cmin, beta / view.width
            );

            /*one_euro_filter(
                raw_points,
                point_times_us,
                num_points,
                0,
                points,
                freq_cmin, beta / view.width
            );*/
        }

        window_clear(win);

        debug_draw_set_view(&view);

        if (WIN_KEY_UP(win, WIN_KEY_SPACE)) {
            //debug_draw_lines(raw_points, num_points, 2.0f, (vec4f){ 1, 1, 1, 1 });

            u32 stroke_start = 0;
            for (u32 i = 0; i < num_strokes; i++) {
                u32 stroke_end = i[stroke_ends];//stroke_ends[i];

                debug_draw_lines(points + stroke_start, stroke_end - stroke_start, 1.0f, (vec4f){ 0, 1, 0, 1 });

                stroke_start = stroke_end;
            }
        } else {
            //debug_draw_circles(raw_points, num_points, 2.0f, (vec4f){ 1, 1, 1, 1 });
            debug_draw_circles(points, num_points, 1.0f, (vec4f){ 0, 1, 0, 1 });
        }

        window_swap_buffers(win);

        u64 frame_end_us = plat_time_usec();
        u64 frame_time_us = frame_end_us - frame_start_us;
        u64 target_frame_us = 1000000 / TARGET_FPS;
        if (frame_time_us < target_frame_us) {
            plat_sleep_ms((u32)(target_frame_us - frame_time_us) / 1000);
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

vec2f screen_to_world(vec2f screen_pos, const win_window* win, const mat3f inv_view_mat) {
    vec2f normalized_pos = {
        (2.0f * screen_pos.x - (f32)win->width) / (f32)win->width,
        -(2.0f * screen_pos.y - (f32)win->height) / (f32)win->height,
    };

    vec2f world_pos = mat3f_mul_vec2f(&inv_view_mat, normalized_pos);

    return world_pos;
}

void one_euro_filter(vec2f* raw_points, u64* times_us, u32 num_points, u32 offset, vec2f* out_points, f32 freq_cmin, f32 beta) {
    for (u32 i = offset; i < num_points; i++) {
        vec2f raw_point = {
            raw_points[i].x,
            raw_points[i].y - RAW_OFFSET_Y
        };

        if (i == 0) {
            out_points[i] = raw_point;
        } else {
            vec2f prev_point = out_points[i - 1];
            u64 prev_time = times_us[i - 1];

            vec2f diff = vec2f_sub(raw_point, prev_point);
            f32 diff_length = vec2f_len(diff);

            f32 time_diff = (f32)(times_us[i] - prev_time) * 1e-6f;

            if (time_diff < 1e-6f) {
                // Avoiding divide by zero
                out_points[i] = raw_point;
            } else {
                f32 speed = diff_length / time_diff;

                f32 freq_cutoff = freq_cmin + beta * speed;
                f32 tau = 1.0f / (2.0f * (f32)PI * freq_cutoff);
                f32 alpha = 1.0f / (1.0f + tau / time_diff);

                out_points[i] = vec2f_add(
                    vec2f_scale(raw_point, alpha),
                    vec2f_scale(out_points[i-1], 1.0f - alpha)
                );
            }
        }
    }
}

