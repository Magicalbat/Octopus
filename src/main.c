
#include "base/base.h"
#include "platform/platform.h"
#include "win/win.h"
#include "debug_draw/debug_draw.h"
#include "truetype/truetype.h"

#include "base/base.c"
#include "platform/platform.c"
#include "win/win.c"
#include "debug_draw/debug_draw.c"
#include "truetype/truetype.c"

void gl_on_error(
    GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar* message, const void* user_param
);

v2_f32 screen_to_world(window* win, view2_f32* view, v2_f32 p);

void test_draw_glyph(
    string8 file, tt_font_info* info,
    u32 codepoint, v2_f32 translate, v2_f32 scale
);

typedef struct {
    v2_f32 translate;
    v2_f32 scale;
    u32 data_offset;
    u32 num_segments;
    u32 num_points;
} instance;

u32 num_glyphs = 0;
v2_f32* vertex_data = NULL;
instance* instance_data = NULL;
u32 glyph_data_size = 0;
u8* glyph_data = NULL;

void push_glyph(
    string8 file, tt_font_info* info,
    u32 codepoint, v2_f32 translate, v2_f32 scale
);

string8 test_vert_source;
string8 test_frag_source;

v2_f32 unpack_point(u32 point_packed) {
    u32 x_bits = (point_packed >>  0) & 0xffff;
    u32 y_bits = (point_packed >> 16) & 0xffff;

    float x = -32768.0f * (float)((x_bits >> 15) & 1) + (float)(x_bits & 0x7FFF);
    float y = -32768.0f * (float)((y_bits >> 15) & 1) + (float)(y_bits & 0x7FFF);

    return (v2_f32){ x, y };
}

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

    string8 fonts[] = {
        STR8_LIT("res/Symbola.ttf"),
        //STR8_LIT("res/comic.ttf"),
        //STR8_LIT("res/Envy Code R.ttf"),
        //STR8_LIT("res/arial.ttf"),
        //STR8_LIT("res/corbeli.ttf"),
        //STR8_LIT("res/Hack.ttf"),
        //STR8_LIT("res/NotoSans-Regular.ttf"),
        //STR8_LIT("res/times.ttf"),
    };

#define NUM_FONTS (sizeof(fonts) / sizeof(fonts[0]))

    string8 font_files[NUM_FONTS] = { 0 };
    tt_font_info font_infos[NUM_FONTS] = { 0 };

    for (u32 i = 0; i < sizeof(fonts) / sizeof(fonts[0]); i++) {
        info_emitf("Parsing %.*s...", STR8_FMT(fonts[i]));
        font_files[i] = plat_file_read(perm_arena, fonts[i]);
        tt_font_init(font_files[i], &font_infos[i]);
    }

    win_gfx_backend_init();
    window* win = win_create(perm_arena, 1280, 720, STR8_LIT("Octopus"));
    win_make_current(win);

#ifndef NDEBUG
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(gl_on_error, NULL);
#endif

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Both in bytes
    u32 glyph_capacity = KiB(128);
    u32 max_glyphs = 256 * NUM_FONTS;

    u32 vert_array, vert_buffer, instance_ssbo, glyph_ssbo;
    u32 shader_prog;
    i32 view_mat_loc;

    glGenVertexArrays(1, &vert_array);
    glBindVertexArray(vert_array);

    vert_buffer = glh_create_buffer(GL_ARRAY_BUFFER, sizeof(v2_f32) * 6 * max_glyphs, NULL, GL_DYNAMIC_DRAW);
    instance_ssbo = glh_create_buffer(GL_SHADER_STORAGE_BUFFER, sizeof(instance) * max_glyphs, NULL, GL_DYNAMIC_DRAW);
    glyph_ssbo = glh_create_buffer(GL_SHADER_STORAGE_BUFFER, glyph_capacity, NULL, GL_DYNAMIC_DRAW);

    num_glyphs = 0;
    vertex_data = PUSH_ARRAY(perm_arena, v2_f32, 6 * max_glyphs);
    instance_data = PUSH_ARRAY(perm_arena, instance, max_glyphs);
    glyph_data_size = 0;
    glyph_data = PUSH_ARRAY(perm_arena, u8, glyph_capacity);

    string8 frag_source = plat_file_read(perm_arena, STR8_LIT("test.glsl"));

    shader_prog = glh_create_shader(test_vert_source, frag_source);

    glUseProgram(shader_prog);
    view_mat_loc = glGetUniformLocation(shader_prog, "u_view_mat");

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

    u32 codepoint_offset = 'a';

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

        if (WIN_KEY_JUST_DOWN(win, WIN_KEY_ARROW_RIGHT)) {
            codepoint_offset++;
        }
        if (WIN_KEY_JUST_DOWN(win, WIN_KEY_ARROW_LEFT)) {
            if (codepoint_offset) codepoint_offset--;
        }

        if (WIN_KEY_DOWN(win, WIN_KEY_ARROW_UP)) {
            codepoint_offset++;
        }
        if (WIN_KEY_DOWN(win, WIN_KEY_ARROW_DOWN)) {
            if (codepoint_offset) codepoint_offset--;
        }

        tt_glyph_data glyph = tt_glyph_data_from_codepoint(
            frame_arena, font_files[0], &font_infos[0], codepoint_offset
        );
        tt_glyph_color_edges(&glyph);

        win_begin_frame(win);

#if 1
        //vertices[0]  = glyph.x_min - 100; vertices[1]  = -glyph.y_max - 100;
        //vertices[2]  = glyph.x_min - 100; vertices[3]  = -glyph.y_min + 100;
        //vertices[4]  = glyph.x_max + 100; vertices[5]  = -glyph.y_min + 100;
        //vertices[6]  = glyph.x_max + 100; vertices[7]  = -glyph.y_min + 100;
        //vertices[8]  = glyph.x_max + 100; vertices[9]  = -glyph.y_max - 100;
        //vertices[10] = glyph.x_min - 100; vertices[11] = -glyph.y_max - 100;

        
        num_glyphs = 0;
        glyph_data_size = 0;

        u32 rows = 6;
        u32 cols = 16;
        for (u32 i = 0; i < NUM_FONTS; i++) {
            f32 font_offset = 160 * (f32)(rows + 1) * (f32)i;

            for (u32 j = 0; j < fonts[i].size; j++) {
                push_glyph(
                    font_files[i], &font_infos[i], fonts[i].str[j],
                    (v2_f32){ 75 * (f32)j, font_offset },
                    (v2_f32){ 100, -100 }
                );
            }

            for (u32 y = 0; y < rows; y++) {
                for (u32 x = 0; x < cols; x++) {
                    u32 codepoint = (y * cols + x) + codepoint_offset;
                    v2_f32 pos = {
                        150 * (f32)x,
                        font_offset + 150 * (f32)(y + 1)
                    };

                    push_glyph(
                        font_files[i], &font_infos[i], codepoint,
                        pos, (v2_f32){ 100, -100 }
                    );
                }
            }
        }

        glBindVertexArray(vert_array);

        printf("%u %u\n", num_glyphs, glyph_data_size);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, glyph_ssbo);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, glyph_data_size, glyph_data);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, glyph_ssbo);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, instance_ssbo);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, num_glyphs * sizeof(instance), instance_data);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, instance_ssbo);

        glBindBuffer(GL_ARRAY_BUFFER, vert_buffer);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v2_f32) * 6 * num_glyphs, vertex_data);

        glUseProgram(shader_prog);
        glUniformMatrix3fv(view_mat_loc, 1, GL_TRUE, view_mat.m);

        glEnableVertexAttribArray(0);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(f32) * 2, NULL);

        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, num_glyphs);

        glDisableVertexAttribArray(0);

#else

        u32* glyph_packed = PUSH_ARRAY_NZ(frame_arena, u32, glyph.num_points + 3);
        memcpy(glyph_packed, glyph.flags, glyph.num_points);
        memcpy(
            (u8*)glyph_packed + ALIGN_UP_POW2(glyph.num_points, 4),
            glyph.points, glyph.num_points * sizeof(v2_i16)
        );

        v2_f32 p = screen_to_world(win, &view, win->mouse_pos);
        f32 min_dist = INFINITY;

        u32 point_offset = ((glyph.num_points + 3) / 4);
        u32 point_index = 0;

        u32 index = 0;
        v2_f32 draw_points[101] = { 0 };
        for (u32 seg = 0; seg < glyph.num_segments; seg++) {
            f32 dist = INFINITY;

            u32 flag = (glyph_packed[point_index / 4] >> ((point_index % 4) * 8)) & 0xff;

            if (flag & TT_POINT_FLAG_LINE) {
                u32 p0_packed = glyph_packed[point_offset + point_index++];
                u32 p1_packed = glyph_packed[point_offset + point_index];

                v2_f32 p0 = unpack_point(p0_packed);
                v2_f32 p1 = unpack_point(p1_packed);

                p0.y *= -1;
                p1.y *= -1;

                draw_points[0].x = p0.x;
                draw_points[0].y = p0.y;
                draw_points[1].x = p1.x;
                draw_points[1].y = p1.y;

                debug_draw_lines(draw_points, 2, 2.0f, (v4_f32){ 1, 1, 1, 1 });

                v2_f32 line_vec = v2_f32_sub(p1, p0);
                v2_f32 point_vec = v2_f32_sub(p, p0);

                f32 t = v2_f32_dot(point_vec, line_vec) / v2_f32_dot(line_vec, line_vec);
                t = CLAMP(t, 0.0f, 1.0f);

                v2_f32 line_point = v2_f32_add(p0, v2_f32_scale(line_vec, t));
                dist = v2_f32_dist(p, line_point);
            } else {
                u32 p0_packed = glyph_packed[point_offset + point_index++];
                u32 p1_packed = glyph_packed[point_offset + point_index++];
                u32 p2_packed = glyph_packed[point_offset + point_index];

                v2_f32 p0 = unpack_point(p0_packed);
                v2_f32 p1 = unpack_point(p1_packed);
                v2_f32 p2 = unpack_point(p2_packed);

                p0.y *= -1;
                p1.y *= -1;
                p2.y *= -1;

                v2_f32 c0 = v2_f32_sub(p, p0);
                v2_f32 c1 = v2_f32_sub(p1, p0);
                v2_f32 c2 = v2_f32_add(p2, v2_f32_add(v2_f32_scale(p1, -2.0f), p0));

                for (u32 i = 0; i <= 10; i++) {
                    f32 t = (f32)i / 10.0f;

                    draw_points[i] = v2_f32_add(
                        v2_f32_add(
                            v2_f32_scale(c2, t * t),
                            v2_f32_scale(c1, 2.0f * t)
                        ), p0
                    ); 
                }

                debug_draw_lines(draw_points, 11, 2.0f, (v4_f32){ 1, 1, 1, 1 });

                f32 a = v2_f32_dot(c2, c2);
                f32 b = 3.0f * v2_f32_dot(c1, c2);
                f32 c = 2.0f * v2_f32_dot(c1, c1) - v2_f32_dot(c2, c0);
                f32 d = -v2_f32_dot(c1, c0);

                f32 ts[3] = { 0 };
                u32 num_t = solve_cubic(ts, a, b, c, d);

                for (u32 i = 0; i < num_t; i++) {
                    f32 t = CLAMP(ts[i], 0.0f, 1.0f);

                    v2_f32 bez_point = v2_f32_add(
                        v2_f32_add(
                            v2_f32_scale(c2, t * t),
                            v2_f32_scale(c1, 2.0f * t)
                        ), p0
                    );

                    f32 cur_dist = v2_f32_dist(p, bez_point);
                    dist = MIN(dist, cur_dist);
                }
            }

            flag = (glyph_packed[point_index / 4] >> ((point_index % 4) * 8)) & 0xff;
            if (flag & TT_POINT_FLAG_CONTOUR_END) {
                point_index++;
            }

            if (dist < min_dist) {
                min_dist = dist;
            }
        }

        if (min_dist < INFINITY) {
            debug_draw_circles(&p, 1, min_dist, (v4_f32){ 0, 1, 0, 0.2f });
        }
#endif

        /*u32 rows = 6;
        u32 cols = 16;
        for (u32 i = 0; i < NUM_FONTS; i++) {
            f32 font_offset = 160 * (f32)(rows + 1) * (f32)i;

            for (u32 j = 0; j < fonts[i].size; j++) {
                test_draw_glyph(
                    font_files[i], &font_infos[i], fonts[i].str[j],
                    (v2_f32){ 75 * (f32)j, font_offset },
                    (v2_f32){ 100, -100 }
                );
            }

            for (u32 y = 0; y < rows; y++) {
                for (u32 x = 0; x < cols; x++) {
                    u32 codepoint = (y * cols + x) + codepoint_offset;
                    v2_f32 pos = {
                        150 * (f32)x,
                        font_offset + 150 * (f32)(y + 1)
                    };

                    test_draw_glyph(
                        font_files[i], &font_infos[i], codepoint,
                        pos, (v2_f32){ 100, -100 }
                    );
                }
            }
        }*/

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

        exit(1);
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

#define BEZ_SUB 5
void test_draw_glyph(
    string8 file, tt_font_info* info,
    u32 codepoint, v2_f32 translate, v2_f32 scale
) {
    if (info == NULL || !info->initialized) { return; }

    f32 units_per_em = (f32)_TT_READ_BE16(file.str + info->head.offset + 18);
    scale = v2_f32_scale(scale, 1.0f / units_per_em);

    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    tt_glyph_data glyph = tt_glyph_data_from_codepoint(scratch.arena, file, info, codepoint);

    v2_f32* points = PUSH_ARRAY_NZ(scratch.arena, v2_f32, glyph.num_points);
    for (u32 i = 0; i < glyph.num_points; i++) {
        points[i] = (v2_f32){ 
            glyph.points[i].x,
            glyph.points[i].y
        };

        points[i] = v2_f32_add(v2_f32_comp_mul(points[i], scale), translate);
    }

    u32 num_draw_points = 0;
    v2_f32* draw_points = PUSH_ARRAY(scratch.arena, v2_f32, glyph.num_points * BEZ_SUB);

    draw_points[num_draw_points++] = points[0];

    u32 index = 0;
    for (u32 seg = 0; seg < glyph.num_segments; seg++) {
        if (glyph.flags[index] & TT_POINT_FLAG_LINE) {
            // Skip over p0 (already in draw list)
            index++;
            v2_f32 p1 = points[index];

            draw_points[num_draw_points++] = p1;
        } else {
            v2_f32 p0 = points[index++];
            v2_f32 p1 = points[index++];
            v2_f32 p2 = points[index];

            for (u32 i = 0; i < BEZ_SUB; i++) {
                f32 t = (f32)(i + 1) / BEZ_SUB;

                v2_f32 m0 = v2_f32_add(p0, v2_f32_scale(v2_f32_sub(p1, p0), t));
                v2_f32 m1 = v2_f32_add(p1, v2_f32_scale(v2_f32_sub(p2, p1), t));

                v2_f32 b = v2_f32_add(m0, v2_f32_scale(v2_f32_sub(m1, m0), t));

                draw_points[num_draw_points++] = b;
            }
        }

        if (glyph.flags[index] & TT_POINT_FLAG_CONTOUR_END) {
            debug_draw_lines(draw_points, num_draw_points, 1.0f, (v4_f32){ 1, 1, 1, 1 });

            num_draw_points = 0;
            index++;

            if (index < glyph.num_points) {
                draw_points[num_draw_points++] = points[index];
            }
        }
    }

    arena_scratch_release(scratch);
}

void push_glyph(
    string8 file, tt_font_info* info,
    u32 codepoint, v2_f32 translate, v2_f32 scale
) {
    if (info == NULL || !info->initialized) { return; }

    f32 units_per_em = (f32)_TT_READ_BE16(file.str + info->head.offset + 18);
    scale = v2_f32_scale(scale, 1.0f / units_per_em);

    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    tt_glyph_data glyph = tt_glyph_data_from_codepoint(scratch.arena, file, info, codepoint);

    u32 vi = num_glyphs * 6;
    vertex_data[vi+0] = (v2_f32){ glyph.x_min - 100, glyph.y_min - 100 };
    vertex_data[vi+1] = (v2_f32){ glyph.x_min - 100, glyph.y_max + 100 };
    vertex_data[vi+2] = (v2_f32){ glyph.x_max + 100, glyph.y_min + 100 };
    vertex_data[vi+3] = (v2_f32){ glyph.x_max + 100, glyph.y_max + 100 };
    vertex_data[vi+4] = (v2_f32){ glyph.x_max + 100, glyph.y_min - 100 };
    vertex_data[vi+5] = (v2_f32){ glyph.x_min - 100, glyph.y_min - 100 };

    for (u32 i = 0; i < 6; i++) {
        vertex_data[vi + i] = v2_f32_add(v2_f32_comp_mul(vertex_data[vi+i], scale), translate);
    }

    u32 data_offset = glyph_data_size;
    instance_data[num_glyphs++] = (instance){
        translate, scale,
        data_offset, glyph.num_segments, glyph.num_points,
    };

    memcpy(glyph_data + data_offset, glyph.flags, glyph.num_points);
    memcpy(
        glyph_data + data_offset + ALIGN_UP_POW2(glyph.num_points, 4),
        glyph.points,
        glyph.num_points * sizeof(v2_i16)
    );

    glyph_data_size += ALIGN_UP_POW2(glyph.num_points, 4) + glyph.num_points * sizeof(v2_i16);
    
    arena_scratch_release(scratch);
}

string8 test_vert_source = GLSL_SOURCE(
    430,

    layout (location = 0) in vec2 a_pos;

    uniform mat3 u_view_mat;

    flat out int instance_id;
    out vec2 pos;

    void main() {
        instance_id = gl_InstanceID;
        pos = a_pos;

        vec2 screen_pos = (u_view_mat * vec3(pos, 1.0)).xy;
        gl_Position = vec4(screen_pos, 0.0, 1.0);
    }
);


