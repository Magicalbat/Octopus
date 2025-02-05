#include "base/base.h"
#include "platform/platform.h"
#include "truetype/truetype.h"
#include "gfx/gfx.h"

#include "base/base.c"
#include "platform/platform.c"
#include "truetype/truetype.c"
#include "gfx/gfx.c"

void gl_on_error(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* user_param);

typedef struct {
    vec4f col;
    vec2f pos;
    f32 radius;
} point_instance_data;

static const char* point_vert_source;
static const char* point_frag_source;

static const char* line_vert_source;
static const char* line_frag_source;

int main(int argc, char** argv) {
    UNUSED(argc);
    UNUSED(argv);

    error_frame_begin();

    plat_init();

    u64 seeds[2] = { 0 };
    plat_get_entropy(seeds, sizeof(seeds));
    prng_seed(seeds[0], seeds[1]);

    mem_arena* perm_arena = arena_create(MiB(64), KiB(264), true);

    gfx_window* win = gfx_win_create(perm_arena, 1280, 720, STR8_LIT("Octopus"));
    gfx_win_make_current(win);
    gfx_win_clear_color(win, (vec4f){ 0.1f, 0.1f, 0.25f, 1.0f });

#ifndef NDEBUG
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(gl_on_error, NULL);
#endif

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    u32 max_points = 0xffff;
    u32 num_points = 0;

    u32 max_strokes = 1024;
    u32 num_strokes = 0;

    point_instance_data* point_data = NULL;
    vec2f* line_data = NULL;
    glh_indirect_draw_cmd* line_indirect_cmds = NULL;

    if (false) {
        mem_arena_temp scratch = arena_scratch_get(NULL, 0);

        string8 file = plat_file_read(scratch.arena, STR8_LIT("res/out.strokes"));

        num_strokes = *(u32*)(file.str);
        max_strokes = num_strokes * 2;

        line_indirect_cmds = ARENA_PUSH_ARRAY(perm_arena, glh_indirect_draw_cmd, max_strokes);

        u32 num_orig_points = *(u32*)(file.str + file.size - 4);
        num_points = num_orig_points + num_strokes;
        max_points = num_points * 2;

        point_data = ARENA_PUSH_ARRAY(perm_arena, point_instance_data, max_points);
        line_data = ARENA_PUSH_ARRAY(perm_arena, vec2f, max_points);

        u32 base_instance = 0;

        u32 cur_point = 0;

        u64 file_pos = 4;
        for (u32 i = 0; i < num_strokes - 2; i++) {
            u32 cur_num_points = *(u32*)(file.str + file_pos);
            //printf("%u\n", cur_num_points);
            file_pos += 4;

            vec2f* points = (vec2f*)(file.str + file_pos);
            file_pos += sizeof(vec2f) * cur_num_points;

            line_indirect_cmds[i] = (glh_indirect_draw_cmd){
                .count = 4,
                .first = 0,
                .base_instance = base_instance,
                .instance_count = cur_num_points
            };
            base_instance += line_indirect_cmds[i].instance_count + 1;

            for (u32 j = 0; j <= cur_num_points; j++) {
                u32 index = j == cur_num_points ? cur_num_points - 2 : j;

                vec2f pos = vec2f_scale(points[index], 4.0f);

                cur_point++;
                point_data[cur_point-1] = (point_instance_data) {
                    //.col = (vec4f){ 0.0f, 0.5f * (1.0f + sinf((f32)i * (f32)PI / 128)), 1.0f, 1.0f },
                    .col = (vec4f){ 1, 0, 0, 1 },
                    .pos = pos,
                    .radius = 0.25f
                };
                line_data[cur_point-1] = pos;
            }
        }

        /*vec2f* positions = (vec2f*)file.str;
        for (u32 i = 0; i < num_orig_points; i++) {
            vec2f pos = vec2f_scale(positions[i], 4.0f);

            point_data[i].col =
                (vec4f){ 0.0f, 0.5f * (1.0f + sinf((f32)i * (f32)PI / 128)), 1.0f, 1.0f };

            point_data[i].pos = pos;

            point_data[i].radius = 1.0f;

            line_data[i] = pos;
        }*/

        arena_scratch_release(scratch);
    }

    if (point_data == NULL) {
        point_data = ARENA_PUSH_ARRAY(perm_arena, point_instance_data, max_points);
        line_data = ARENA_PUSH_ARRAY(perm_arena, vec2f, max_points);
        line_indirect_cmds = ARENA_PUSH_ARRAY(perm_arena, glh_indirect_draw_cmd, max_strokes);
    }

    struct {
        u32 program;
        i32 view_mat_loc;
    } point_shader = { 0 };

    point_shader.program = glh_create_shader(point_vert_source, point_frag_source);
    glUseProgram(point_shader.program);
    point_shader.view_mat_loc = glGetUniformLocation(point_shader.program, "u_view_mat");

    f32 point_pattern_verts[] = {
        -1.0f, -1.0f, 
         1.0f, -1.0f, 
        -1.0f,  1.0f, 

        -1.0f,  1.0f, 
         1.0f, -1.0f, 
         1.0f,  1.0f, 
    };

    u32 point_vert_array = 0;
    glGenVertexArrays(1, &point_vert_array);
    glBindVertexArray(point_vert_array);

    u32 point_pattern_buffer = glh_create_buffer(GL_ARRAY_BUFFER, sizeof(point_pattern_verts), point_pattern_verts, GL_STATIC_DRAW);
    u32 point_instance_buffer = glh_create_buffer(GL_ARRAY_BUFFER, sizeof(point_instance_data) * max_points, NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(point_instance_data) * num_points, point_data);

    struct {
        u32 program;
        i32 view_mat_loc;
        i32 radius_loc;
        i32 geom_scale_loc;
        i32 col_loc;
    } line_shader = { 0 };

    line_shader.program = glh_create_shader(line_vert_source, line_frag_source);
    glUseProgram(line_shader.program);
    line_shader.view_mat_loc = glGetUniformLocation(line_shader.program, "u_view_mat");
    line_shader.radius_loc = glGetUniformLocation(line_shader.program, "u_radius");
    line_shader.geom_scale_loc = glGetUniformLocation(line_shader.program, "u_geom_scale");
    line_shader.col_loc = glGetUniformLocation(line_shader.program, "u_col");

    u32 line_vert_array = 0;
    glGenVertexArrays(1, &line_vert_array);
    glBindVertexArray(point_vert_array);

    u32 line_buffer = glh_create_buffer(GL_ARRAY_BUFFER, sizeof(vec2f) * max_points, NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec2f) * num_points, line_data);

    u32 stroke_num_points = 0;

    u32 line_indirect_buffer = glh_create_buffer(
        GL_DRAW_INDIRECT_BUFFER,
        sizeof(glh_indirect_draw_cmd) * max_strokes,
        NULL, GL_STREAM_DRAW
    );
    glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, sizeof(glh_indirect_draw_cmd) * num_strokes, line_indirect_cmds);

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

    b32 draw_points = true;

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

        if (GFX_IS_KEY_JUST_DOWN(win, GFX_KEY_R)) {
            num_points = 0;
            num_strokes = 0;
        }

        if (GFX_IS_MOUSE_JUST_DOWN(win, GFX_MB_LEFT)) {
            if (num_strokes < max_strokes) {
                stroke_num_points = 0;
                num_strokes++;

                line_indirect_cmds[num_strokes-1] = (glh_indirect_draw_cmd){
                    .count = 4,
                    .instance_count = 0,
                    .first = 0,
                    .base_instance = num_points,
                };
            }
        }

        u32 points_written = 0;
        if (GFX_IS_MOUSE_DOWN(win, GFX_MB_LEFT)) {
            for (u32 i = 0; i < win->mouse_pos_cache_size && num_points < max_points; i++) {
                vec2f win_pos = win->mouse_pos_cache[i];
                vec2f normalized_pos = {
                    2.0f * win_pos.x / (f32)win->width - 1.0f,
                    -(2.0f * win_pos.y / (f32)win->height - 1.0f),
                };
                vec2f pos = mat3f_mul_vec2f(&inv_view_mat, normalized_pos);

                if (num_points > 0) {
                    vec2f prev_pos = point_data[num_points-1].pos;
                    f32 dist = vec2f_dist(prev_pos, pos);

                    if (dist < 0.25f) {
                        continue;
                    }
                }

                stroke_num_points++;
                if (stroke_num_points >= 2) {
                    line_indirect_cmds[num_strokes-1].instance_count = stroke_num_points - 1;
                }

                points_written++;
                num_points++;
                point_data[num_points-1] = (point_instance_data) {
                    //(vec4f){ 0.0f, 0.5f * (1.0f + sinf((f32)num_points * (f32)PI / 128)), 1.0f, 1.0f },
                    (vec4f){ 1, 0, 0, 1 },
                    pos,
                    0.25f
                };
                line_data[num_points-1] = pos;
            }
        }

        if (GFX_IS_KEY_JUST_DOWN(win, GFX_KEY_SPACE)) {
            draw_points = !draw_points;
        }

        gfx_win_clear(win);

        if (points_written) {
            glBindBuffer(GL_ARRAY_BUFFER, point_instance_buffer);
            glBufferSubData(
                GL_ARRAY_BUFFER,
                sizeof(point_instance_data) * (num_points - points_written),
                sizeof(point_instance_data) * points_written,
                &point_data[num_points - points_written]
            );

            glBindBuffer(GL_ARRAY_BUFFER, line_buffer);
            glBufferSubData(
                GL_ARRAY_BUFFER,
                sizeof(vec2f) * (num_points - points_written),
                sizeof(vec2f) * points_written,
                &line_data[num_points - points_written]
            );

            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, line_indirect_buffer);
            glBufferSubData(
                GL_DRAW_INDIRECT_BUFFER,
                sizeof(glh_indirect_draw_cmd) * (num_strokes - 1),
                sizeof(glh_indirect_draw_cmd),
                &line_indirect_cmds[num_strokes-1]
            );
        }

        // Drawing lines
        {
            f32 radius = 0.5f;
            f32 geom_scale = 1.0f;

            {
                f32 screen_radius = radius * ((f32)win->width / view.width);
                f32 adjusted_screen_radius = screen_radius + (sqrtf(2.0f));
                geom_scale = adjusted_screen_radius / screen_radius;
            }

            glUseProgram(line_shader.program);
            glUniformMatrix3fv(line_shader.view_mat_loc, 1, GL_TRUE, view_mat.m);
            glUniform1f(line_shader.radius_loc, radius);
            glUniform1f(line_shader.geom_scale_loc, geom_scale);
            glUniform4f(line_shader.col_loc, 1, 1, 1, 1);

            glBindVertexArray(line_vert_array);

            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);

            glVertexAttribDivisor(0, 1);
            glVertexAttribDivisor(1, 1);

            glBindBuffer(GL_ARRAY_BUFFER, line_buffer);

            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, (GLsizei)(sizeof(vec2f)), (void*)(0));
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, (GLsizei)(sizeof(vec2f)), (void*)(sizeof(vec2f)));

            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, line_indirect_buffer);
            glMultiDrawArraysIndirect(GL_TRIANGLE_STRIP, 0, (GLsizei)num_strokes, sizeof(glh_indirect_draw_cmd));
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

            glVertexAttribDivisor(0, 0);
            glVertexAttribDivisor(1, 0);

            glDisableVertexAttribArray(0);
            glDisableVertexAttribArray(1);
        }

        if (draw_points) {
            glUseProgram(point_shader.program);
            glUniformMatrix3fv(point_shader.view_mat_loc, 1, GL_TRUE, view_mat.m);

            glBindVertexArray(point_vert_array);

            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);
            glEnableVertexAttribArray(2);
            glEnableVertexAttribArray(3);

            glBindBuffer(GL_ARRAY_BUFFER, point_pattern_buffer);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

            glBindBuffer(GL_ARRAY_BUFFER, point_instance_buffer);

            glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(point_instance_data), (void*)(offsetof(point_instance_data, col)));
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(point_instance_data), (void*)(offsetof(point_instance_data, pos)));
            glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(point_instance_data), (void*)(offsetof(point_instance_data, radius)));

            glVertexAttribDivisor(1, 1);
            glVertexAttribDivisor(2, 1);
            glVertexAttribDivisor(3, 1);

            glDrawArraysInstanced(GL_TRIANGLES, 0, 6, (GLsizei)(num_points));

            glVertexAttribDivisor(1, 0);
            glVertexAttribDivisor(2, 0);
            glVertexAttribDivisor(3, 0);

            glDisableVertexAttribArray(0);
            glDisableVertexAttribArray(1);
            glDisableVertexAttribArray(2);
            glDisableVertexAttribArray(3);
        }

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

static const char* point_vert_source = GLSL_SOURCE(
    330,

    layout (location = 0) in vec2 a_pos_pattern;
    layout (location = 1) in vec4 a_col;
    layout (location = 2) in vec2 a_pos;
    layout (location = 3) in float a_radius;

    out vec2 sdf_pos;
    out vec4 col;

    uniform mat3 u_view_mat;

    void main() {
        col = a_col;
        sdf_pos = a_pos_pattern;
        vec2 pos = (u_view_mat * vec3(a_pos_pattern * a_radius + a_pos, 1)).xy;
        gl_Position = vec4(pos, 0.0, 1.0);
    }
);

static const char* point_frag_source = GLSL_SOURCE(
    330,

    layout (location = 0) out vec4 out_col;

    in vec2 sdf_pos;
    in vec4 col;

    void main() {
        float dist = length(sdf_pos) - 1.0;
        float blending = fwidth(dist);
        float alpha = smoothstep(blending, -blending, dist);
        out_col = vec4(col.xyz, col.w * alpha);
    }
);

static const char* line_vert_source = GLSL_SOURCE(
    330,

    layout (location = 0) in vec2 a_p0;
    layout (location = 1) in vec2 a_p1;

    out vec2 point_to_line;
    out vec2 line_vec;

    uniform mat3 u_view_mat;
    uniform float u_radius;
    uniform float u_geom_scale;

    void main() {
        float edge = ((gl_VertexID / 2) * 2 - 1);

        line_vec = a_p1 - a_p0;
        vec2 norm_line_vec = normalize(line_vec);
        vec2 norm = vec2(-norm_line_vec.y, norm_line_vec.x);
        
        float geom_radius = u_radius * u_geom_scale;
        vec2 end_cap_offset = -norm_line_vec * geom_radius;

        vec2 world_pos = a_p0 + end_cap_offset +
            (line_vec - end_cap_offset) * (gl_VertexID % 2) +
            norm * geom_radius * edge;

        point_to_line = world_pos - a_p0;

        vec2 pos = (u_view_mat * vec3(world_pos, 1)).xy;
        gl_Position = vec4(pos, 0.0, 1.0);
    }
);

static const char* line_frag_source = GLSL_SOURCE(
    330,

    layout (location = 0) out vec4 out_col;

    in vec2 point_to_line;
    in vec2 line_vec;

    uniform vec4 u_col;
    uniform float u_radius;
    
    void main() {
        float t = max(dot(point_to_line, line_vec) / dot(line_vec, line_vec), 0.0);
        float dist = length(point_to_line - line_vec * t) - u_radius;
        float blending = fwidth(dist);
        float alpha = smoothstep(blending, -blending, dist);
        out_col = vec4(u_col.xyz, u_col.w * alpha);
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

