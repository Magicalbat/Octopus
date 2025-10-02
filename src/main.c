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

static const char* sdf_vert_source;
static const char* sdf_frag_source;

int main(int argc, char** argv) {
    UNUSED(argc);
    UNUSED(argv);

    error_frame_begin();

    plat_init();

    u64 seeds[2] = { 0 };
    plat_get_entropy(seeds, sizeof(seeds));
    prng_seed(seeds[0], seeds[1]);

    mem_arena* perm_arena = arena_create(MiB(64), KiB(264), true);

    string8 font_file = plat_file_read(perm_arena, STR8_LIT("res/NotoSans-Regular.ttf"));
    tt_font_info font = { 0 };
    tt_init_font(font_file, &font);

    u32 codepoint = 'a';

    tt_segment* segments = ARENA_PUSH_ARRAY(perm_arena, tt_segment, font.max_glyph_points);
    u32 num_bez_segments = 0;
    u32 num_segments = tt_get_glyph_outline(
        &font,
        tt_get_glyph_index(&font, codepoint),
        segments,
        0,
        (mat2f){ .m = {
            1, 0,
            0, -1
        } },
        (vec2f){ 0, 500 }
    );

    u32 max_points = 4096;
    u32 num_points = 0;
    vec2f* points = ARENA_PUSH_ARRAY(perm_arena, vec2f, max_points);

    win_window* win = window_create(perm_arena, 1280, 720, STR8_LIT("Octopus"));
    window_make_current(win);
    window_clear_color(win, (vec4f){ 0.1f, 0.1f, 0.25f, 1.0f });

#ifndef NDEBUG
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(gl_on_error, NULL);
#endif

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    u32 sdf_vert_array = 0;
    u32 sdf_vert_buffer = 0;
    u32 sdf_shader_prog = 0;
    i32 sdf_dist_radius_loc = 0;
    i32 sdf_view_mat_loc = 0;

    glGenVertexArrays(1, &sdf_vert_array);
    sdf_vert_buffer = glh_create_buffer(GL_ARRAY_BUFFER, sizeof(vec2f) * max_points, NULL, GL_DYNAMIC_DRAW);

    sdf_shader_prog = glh_create_shader(sdf_vert_source, sdf_frag_source);
    glUseProgram(sdf_shader_prog);

    sdf_dist_radius_loc = glGetUniformLocation(sdf_shader_prog, "u_dist_radius");
    sdf_view_mat_loc = glGetUniformLocation(sdf_shader_prog, "u_view_mat");

    num_points = 0;
    num_bez_segments = 0;
    for (u32 i = 0; i < num_segments; i++) {
        if (segments[i].type == TT_SEGMENT_QBEZIER) {
            vec2f p0 = segments[i].qbez.p0;
            vec2f p1 = segments[i].qbez.p1;
            vec2f p2 = segments[i].qbez.p2;

            points[num_points++] = p0;
            points[num_points++] = p1;
            points[num_points++] = p2;

            num_bez_segments++;
        }
        // TODO: handle line segments
    }

    glBindVertexArray(sdf_vert_array);
    glBindBuffer(GL_ARRAY_BUFFER, sdf_vert_buffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, num_points * sizeof(vec2f), points);

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

    i64 motion0_touch_id = -1;
    vec2f motion0_touch_pos = { 0 };
    i64 motion1_touch_id = -1;
    f32 motion01_dist = 0.0f;

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

        b32 update_glyph = false;

        if (WIN_KEY_JUST_DOWN(win, WIN_KEY_ARROW_LEFT)) {
            codepoint--;
            update_glyph = true;
        }
        if (WIN_KEY_JUST_DOWN(win, WIN_KEY_ARROW_RIGHT)) {
            codepoint++;
            update_glyph = true;
        }

        if (WIN_KEY_DOWN(win, WIN_KEY_A)) {
            codepoint--;
            update_glyph = true;
        }
        if (WIN_KEY_DOWN(win, WIN_KEY_D)) {
            codepoint++;
            update_glyph = true;
        }

        if (update_glyph) {
            num_segments = tt_get_glyph_outline(
                &font,
                tt_get_glyph_index(&font, codepoint),
                segments,
                0,
                (mat2f){ .m = {
                    1, 0,
                    0, -1
                } },
                (vec2f){ 0, 0 }
            );
        }
        
        mat3f_from_view(&view_mat, view);
        mat3f_from_inv_view(&inv_view_mat, view);

        window_clear(win);

        debug_draw_set_view(&view);

        glUseProgram(sdf_shader_prog);
        glUniformMatrix3fv(sdf_view_mat_loc, 1, GL_TRUE, view_mat.m);
        glUniform1f(sdf_dist_radius_loc, 10.0f);

        glBindVertexArray(sdf_vert_array);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        glVertexAttribDivisor(0, 1);
        glVertexAttribDivisor(1, 1);
        glVertexAttribDivisor(2, 1);

        glBindBuffer(GL_ARRAY_BUFFER, sdf_vert_buffer);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2f) * 3, (void*)0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vec2f) * 3, (void*)(sizeof(vec2f)));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vec2f) * 3, (void*)(sizeof(vec2f) * 2));

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LEQUAL);

        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 6, (GLsizei)num_bez_segments);

        glDisable(GL_DEPTH_TEST);

        glVertexAttribDivisor(0, 0);
        glVertexAttribDivisor(1, 0);
        glVertexAttribDivisor(2, 0);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);


        /*for (u32 i = 0; i < num_segments; i++) {
            if (segments[i].type == TT_SEGMENT_LINE) {
                points[num_points++] = segments[i].line.p0;
                points[num_points++] = segments[i].line.p1;
            } else {
                for (f32 t = 0; t <= 1; t += 0.1f) {
                    points[num_points++] = tt_segment_point(&segments[i], t);
                }
            }

            if (segments[(i + 1) % num_segments].flags & TT_SEGMENT_FLAG_CONTOUR_START) {
                debug_draw_lines(points, num_points, 5.0f, (vec4f){ 1, 1, 1, 1 });
                num_points = 0;
            }
        }*/

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

    glDeleteProgram(sdf_shader_prog);
    glDeleteBuffers(1, &sdf_vert_buffer);
    glDeleteVertexArrays(1, &sdf_vert_array);

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

#undef min
#undef max

static const char* sdf_vert_source = GLSL_SOURCE(
    330,

    layout (location = 0) in vec2 a_p0;
    layout (location = 1) in vec2 a_p1;
    layout (location = 2) in vec2 a_p2;

    uniform float u_dist_radius;
    uniform mat3 u_view_mat;

    flat out float dist_scale;

    out vec2 world_pos;
    flat out vec2 p0;
    flat out vec2 p1;
    flat out vec2 p2;

    void main() {
        dist_scale = 1.0 / u_dist_radius;

        p0 = a_p0;
        p1 = a_p1;
        p2 = a_p2;

        vec2 min_p = min(a_p0, a_p2);
        vec2 max_p = max(a_p0, a_p2);

        // Param of local extrema
        vec2 t = clamp((a_p0 - a_p1) / (a_p0 - 2.0 * a_p1 + a_p2), 0.0, 1.0);
        vec2 s = 1.0 - t;
        vec2 q = s * s * a_p0 + 2.0 * s * t * a_p1 + t * t * a_p2;
        min_p = min(min_p, q);
        max_p = max(max_p, q);

        min_p -= u_dist_radius;
        max_p += u_dist_radius;

        vec2 diff = max_p - min_p;

        world_pos = min_p + vec2(gl_VertexID % 2, gl_VertexID >= 2) * diff;
        
        vec2 screen_pos = (u_view_mat * vec3(world_pos, 1.0)).xy;
        gl_Position = vec4(screen_pos, 0.0, 1.0);
    }
);

static const char* sdf_frag_source = GLSL_SOURCE(
    330,

    layout (location = 0) out vec4 out_col;

    flat in float dist_scale;

    in vec2 world_pos;
    flat in vec2 p0;
    flat in vec2 p1;
    flat in vec2 p2;

    float cross2d(vec2 a, vec2 b) {
        return a.x * b.y - a.y * b.x;
    }

    void main() {
        vec2 a = p1 - p0;
        vec2 b = p0 - 2.0*p1 + p2;
        vec2 c = a * 2.0;
        vec2 d = p0 - world_pos;

        // cubic to be solved (kx*=3 and ky*=3)
        float kk = 1.0/dot(b,b);
        float kx = kk * dot(a,b);
        float ky = kk * (2.0*dot(a,a)+dot(d,b))/3.0;
        float kz = kk * dot(d,a);      

        float res = 0.0;
        float sgn = 0.0;

        float p  = ky - kx*kx;
        float q  = kx*(2.0*kx*kx - 3.0*ky) + kz;
        float p3 = p*p*p;
        float q2 = q*q;
        float h  = q2 + 4.0*p3;


        if( h>=0.0 ) 
        {   // 1 root
            h = sqrt(h);
            
            h = (q<0.0) ? h : -h; // copysign()
            float x = (h-q)/2.0;
            float v = sign(x)*pow(abs(x),1.0/3.0);
            float t = v - p/v;

            // from NinjaKoala - single newton iteration to account for cancellation
            t -= (t*(t*t+3.0*p)+q)/(3.0*t*t+3.0*p);
            
            t = clamp( t-kx, 0.0, 1.0 );
            vec2  w = d+(c+b*t)*t;
            res = dot(w, w);
            sgn = cross2d(c+2.0*b*t,w);
        }
        else 
        {   // 3 roots
            float z = sqrt(-p);

            float v = acos(q/(p*z*2.0))/3.0;
            float m = cos(v);
            float n = sin(v);

            n *= sqrt(3.0);
            vec3 t = clamp( vec3(m+m,-n-m,n-m)*z-kx, 0.0, 1.0 );
            vec2 qx=d+(c+b*t.x)*t.x; float dx=dot(qx, qx); float sx=cross2d(a+b*t.x,qx);
            vec2 qy=d+(c+b*t.y)*t.y; float dy=dot(qy, qy); float sy=cross2d(a+b*t.y,qy);
            if( dx<dy ) {res=dx;sgn=sx;} else {res=dy;sgn=sy;}
        }
        
        //return sqrt( res )*sign(sgn);
        //float dist = res - 3.0;

        //float blending = fwidth(dist);
        //float alpha = smoothstep(blending, -blending, dist);
        //out_col = vec4(1, 1, 1, max(alpha, 0.1));

        float dist = min(1.0, sqrt(res) * dist_scale);
        gl_FragDepth = dist;
        float col = 1.0 - dist;
        out_col = vec4(vec3(col), 1.0);
    }
);

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
            raw_points[i].y
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

